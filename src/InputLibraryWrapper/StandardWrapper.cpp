#include "StandardWrapper.h"

#include "AtomicWrite.h"
#include "Common.h"
#include "LoudnessFile.h"
#include "LibSNDWrapper.h"

#include <utility> // std::swap
#include <cstdio> // std::tmpfile
#include <unistd.h> // _POSIX_MAPPED_FILES
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h> // posix_fallocate()
#include <linux/version.h>
#include <cstring>

template class StandardWrapper<int16_t>;
template class StandardWrapper<int32_t>;
template class StandardWrapper<float>;
template class StandardWrapper<sndfile_sample_t>;

// only required for unit testing
template class StandardWrapper<double>;
template class StandardWrapper<uint8_t>;


template<typename SAMPLEFORMAT>
StandardWrapper<SAMPLEFORMAT>::StandardWrapper(string filename)
: Song(filename)
{
    this->init();
}

template<typename SAMPLEFORMAT>
StandardWrapper<SAMPLEFORMAT>::StandardWrapper(string filename, Nullable<size_t> offset, Nullable<size_t> len)
: Song(filename, offset, len)
{
    this->init();
}

template<typename SAMPLEFORMAT>
void StandardWrapper<SAMPLEFORMAT>::init() noexcept
{
    this->gainCorrection = LoudnessFile::read(this->Filename);
}

template<typename SAMPLEFORMAT>
StandardWrapper<SAMPLEFORMAT>::~StandardWrapper()
{
}

template<typename SAMPLEFORMAT>
SAMPLEFORMAT* StandardWrapper<SAMPLEFORMAT>::allocPcmBuffer(size_t items) noexcept
{
#if _POSIX_MAPPED_FILES && _POSIX_C_SOURCE >= 200112L

    size_t length = items * sizeof(SAMPLEFORMAT);

    {
        int fd = -1;
        int flags = MAP_ANONYMOUS | MAP_PRIVATE;
        std::unique_ptr<std::FILE, decltype(&std::fclose)> tmp(nullptr, std::fclose);
// uncomment to use a file backed mapping
#if 0
        tmp.reset(std::tmpfile());
        if (tmp != nullptr)
        {
            fd = ::fileno(tmp.get());
            int ret = ::posix_fallocate(fd, 0, length);
            if (ret == 0)
            {
                flags = MAP_SHARED;
            }
            else
            {
                fd = -1;
                CLOG(LogLevel_t::Debug, "posix_fallocate() failed: " << strerror(errno));
            }
        }
        else
        {
            CLOG(LogLevel_t::Debug, "std::tmpfile() failed: " << strerror(errno));
        }
#endif
        // Prefer to use mmap, because it ensures that the application always sees zero initialized
        // memory. It also supports backing files, which however might shortening life of SSDs, thus
        // it's disabled for now
        void* data = ::mmap(nullptr, length, PROT_READ | PROT_WRITE, flags | MAP_HUGETLB, fd, 0);
        if (data == MAP_FAILED)
        {
            CLOG(LogLevel_t::Debug, "MAP_HUGETLB failed, trying again without it: " << strerror(errno));
            // try again without huge pages
            data = ::mmap(nullptr, length, PROT_READ | PROT_WRITE, flags, fd, 0);
        }
        if (data != MAP_FAILED)
        {
            if (::madvise(data, length, MADV_DONTDUMP) != 0)
            {
                CLOG(LogLevel_t::Debug, "madvise(MADV_DONTDUMP) failed: " << strerror(errno));
            }

            this->backingFile = tmp.release();
            return static_cast<SAMPLEFORMAT*>(data);
        }
        else
        {
            CLOG(LogLevel_t::Debug, "mmap() failed: " << strerror(errno));
        }
    } // tmp is being closed and will be deleted after closed

    return nullptr;
#else

    // alternative attempt, pure new alloc
    return new (std::nothrow) SAMPLEFORMAT[items];
#endif
}

/**
 * @brief manages that Song::data holds new PCM
 *
 * this method trys to alloc a buffer that is big enough to hold the whole PCM of whatever audiofile in memory
 * if this fails it trys to allocate a buffer big enough to hold gConfig.FramesToRender frames and fills this with new PCM on each call
 *
 * if even that allocation fails, an exception will be thrown
 */
template<typename SAMPLEFORMAT>
void StandardWrapper<SAMPLEFORMAT>::fillBuffer()
{
    const auto Channels = this->Format.Channels();
    const auto TotalFrames = this->getFrames();

    if (this->count == static_cast<size_t>(TotalFrames) * Channels)
    {
        // Song::data already filled up with all the audiofile's PCM, nothing to do here (most likely case)
        return;
    }
    else if (this->count == 0) // no buffer allocated?
    {
        if (!this->Format.IsValid())
        {
            THROW_RUNTIME_ERROR("Failed to allocate buffer: SongFormat not valid!");
        }

        // usually this shouldnt block at all, since we only end up here after releaseBuffer() was called
        // and releaseBuffer already waits for the render thread to finish... however it doesnt hurt
        WAIT(this->futureFillBuffer);

        size_t itemsToAlloc = 0;
        if (gConfig.RenderWholeSong)
        {
            itemsToAlloc = TotalFrames * Channels;

            // try to alloc a buffer to hold the whole song's pcm in memory
            this->data = this->allocPcmBuffer(itemsToAlloc);
            if (this->data != nullptr) // buffer successfully allocated, fill it asynchronously
            {
                this->count = itemsToAlloc;

                // (pre-)render the first few milliseconds
                frame_t firstFrames = (gConfig.PreRenderTime == 0) ? TotalFrames : msToFrames(gConfig.PreRenderTime, this->Format.SampleRate);
                this->render(this->data, Channels, firstFrames);

                // immediatly start filling the rest of the pcm buffer
                frame_t restFrames = TotalFrames - this->framesAlreadyRendered;
                if(restFrames > 0)
                {
                    /* advance the pcm pointer by that many items where we previously ended filling it */
                    SAMPLEFORMAT *pcm = static_cast<SAMPLEFORMAT *>(this->data);
                    pcm += (this->framesAlreadyRendered * Channels);

                    this->futureFillBuffer = async(launch::async, &StandardWrapper::renderAsync, this, pcm, Channels, restFrames /*render everything*/);

                    // allow the render thread to do his work
                    this_thread::yield();
                }
                return;
            }
        }

        // well either we shall not render whole song once or something went wrong during alloc (not enough memory??)
        // so try to alloc at least enough to do double buffering
        // if this fails too, an exception will be thrown
        itemsToAlloc = gConfig.FramesToRender * Channels;

        try
        {
            auto len = itemsToAlloc * 2;
            SAMPLEFORMAT* tmp = new SAMPLEFORMAT[len];
            this->data = tmp;
            this->preRenderBuf = tmp + itemsToAlloc;
            this->count = itemsToAlloc;

            len *= sizeof(SAMPLEFORMAT);
            if(!::PageLockMemory(tmp, len))
            {
                CLOG(LogLevel_t::Info, "Failed to page-lock " << len << " bytes of memory, swapping possible." << endl);
            }
            else
            {
                CLOG(LogLevel_t::Debug, "Successfully page-locked " << len << " bytes of memory." << endl);
            }
        }
        catch (const std::bad_alloc &e)
        {
            this->releaseBuffer();
            throw;
        }

        this->render(this->data, Channels, gConfig.FramesToRender);
    }
    else // only small buffer allocated, i.e. this->count == gConfig.FramesToRender * this->Format.Channels
    {
        WAIT(this->futureFillBuffer);

        // data is the consumed pcm buffer, preRenderBuf holds fresh pcm
        std::swap(this->data, this->preRenderBuf);
    }

    this->futureFillBuffer = async(launch::async, &StandardWrapper::renderAsync, this, this->preRenderBuf, Channels, gConfig.FramesToRender);
}

/**
 * The purpose of renderAsync is to forward the polymorphic call to this->render(), since we cannot make polymorphic calls in std::async().
 */
template<typename SAMPLEFORMAT>
void StandardWrapper<SAMPLEFORMAT>::renderAsync(pcm_t *const bufferToFill, const uint32_t Channels, frame_t framesToRender)
{
    this->render(bufferToFill, Channels, framesToRender);

#if _POSIX_MAPPED_FILES && _POSIX_C_SOURCE >= 200112L && LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
    if (this->preRenderBuf == nullptr)
    {
        // If we allocated a PCM buffer for the whole file, advice the kernel to free related pages when the system comes under memory pressure.
        // This avoids triggering the OOM killer and prevents potentially heavy disk activity leading to system unresponsiveness.
        //
        // To achieve this, we use the Linux specific MADV_FREE. On a proper POSIX compliant OS one would use MADV_DONTNEED. Linux,
        // unfortunately implements MADV_DONTNEED in the same broken way as TRU64. See the following links for further reading:
        //
        // http://linux-kernel.2935.n7.nabble.com/wrong-madvise-MADV-DONTNEED-semantic-td18033.html
        // https://www.youtube.com/watch?v=bg6-LVCHmGM&feature=youtu.be&t=3518
        // https://stackoverflow.com/q/14968309
        //
        // Although MADV_FREE is available since linux 4.5, we require at least 4.12, because (madvise man page):
        // "In Linux before version 4.12, when freeing pages on a swapless system, the pages in the given range are freed instantly, regardless of memory pressure."
        if (::madvise(this->data, this->count * sizeof(SAMPLEFORMAT), MADV_FREE) != 0)
        {
            CLOG(LogLevel_t::Debug, "madvise(MADV_FREE) failed: " << strerror(errno));
        }
    }
#endif
}

template<typename SAMPLEFORMAT>
void StandardWrapper<SAMPLEFORMAT>::releaseBuffer() noexcept
{
    this->stopFillBuffer = true;
    WAIT(this->futureFillBuffer);

#if _POSIX_MAPPED_FILES && _POSIX_C_SOURCE >= 200112L
    if (this->data != nullptr)
    {
        if (::munmap(this->data, this->count * sizeof(SAMPLEFORMAT)) != 0)
        {
            CLOG(LogLevel_t::Error, "munmap() failed: " << strerror(errno));
        }
    }
    this->data = nullptr;
#endif

    if (this->backingFile != nullptr)
    {
        fclose(this->backingFile);
        this->backingFile = nullptr;
    }
    else
    {
        if(this->preRenderBuf != nullptr)
        {
            ::PageUnlockMemory(this->data, this->count * 2 * sizeof(SAMPLEFORMAT));

            // make sure we pass the pointer returned by new[] to delete[], i.e. the one pointing to the beginning of the alloced buffer
            if(this->data > this->preRenderBuf)
            {
                std::swap(this->data, this->preRenderBuf);
            }
        }
    }

    delete[] static_cast<SAMPLEFORMAT *>(this->data);
    this->data = nullptr;
    this->preRenderBuf = nullptr;
    this->count = 0;
    this->framesAlreadyRendered = 0;

    this->stopFillBuffer = false;
}

template<typename SAMPLEFORMAT>
frame_t StandardWrapper<SAMPLEFORMAT>::getFramesRendered() const noexcept
{
    return this->framesAlreadyRendered;
}
