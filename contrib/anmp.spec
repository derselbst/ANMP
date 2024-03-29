
# clang <= 3.7 doesnt build with build-id
%if %{defined fedora}
%undefine _missing_build_ids_terminate_build
%endif

%define builddir build

Name: anmp
Version: 10
Release: 0
License: GPL-2.0
Summary: Another Nameless Music Player
Url: https://www.github.com/derselbst/ANMP
Group: Development/Libraries/C and C++
Source0: anmp-%{version}.tar.bz2

%define sffile OldSeiterPiano.sf2
Source1: %{sffile}

BuildRoot: %{_tmppath}/%{name}-%{version}-build

%if 0%{?mageia}
BuildRequires: cmake >= 1:3.1.0
%else
BuildRequires: cmake >= 3.1.0
%endif

BuildRequires: pkgconfig(fluidsynth) >= 2.2.0
BuildRequires: pkgconfig(smf)
BuildRequires: libaopsf-devel
BuildRequires: libcue-devel
BuildRequires: libgme-devel >= 0.6.3
BuildRequires: libmad-devel pkgconfig(id3tag)
BuildRequires: pkgconfig(sndfile)
BuildRequires: libmodplug-devel
BuildRequires: vgmstream-devel
%ifarch x86_64 i586 i686
BuildRequires: lazyusf2-devel
%endif

# SuSE specific:
# everything newer openSUSE Leap 15.0 or openSUSE Tumbleweed
%if 0%{?sle_version} >= 150000 || 0%{?suse_version} > 1500
BuildRequires: ffmpeg-4-libavcodec-devel
BuildRequires: ffmpeg-4-libavformat-devel
BuildRequires: ffmpeg-4-libavutil-devel
BuildRequires: ffmpeg-4-libswresample-devel

BuildRequires: gcc10
BuildRequires: gcc10-c++

BuildRequires: update-desktop-files
%endif

# CentOS specific:
%if %{defined centos_version}
%endif

# Fedora specific:
%if %{defined fedora}
%endif

# Mageia specific:
%if %{defined mageia}
%endif

# RedHat specific:
%if %{defined rhel_version}
%endif

BuildRequires: pkgconfig(alsa)
BuildRequires: pkgconfig(jack) pkgconfig(samplerate)
BuildRequires: pkgconfig(portaudio-2.0)
BuildRequires: libebur128-devel

BuildRequires: pkgconfig(Qt5Widgets)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5OpenGL)


%description
A MusicPlayer capable of playing various formats.

The key features are:
  - gapless playback
  - cue sheets
  - arbitrary (forward) looping of songs
  - easy attempt to implement new formats

%package        progs
Summary:        Programs for %{name}
Group:          Development/Libraries/C and C++
Requires:       anmp

%description    progs
Additional useful tools for %{name}


%prep
%setup -q

%build
mkdir -p %{builddir}
cd %{builddir}

%if 0%{?suse_version}
export CC=gcc-10
export CXX=g++-10
%endif

cmake .. \
        -DFLUIDSYNTH_DEFAULT_SF2=%{_datadir}/%{name}/%{sffile} \
        -DCMAKE_INSTALL_PREFIX:PATH=%{_prefix} \
        -DINCLUDE_INSTALL_DIR:PATH=%{_includedir} \
        -DLIB_INSTALL_DIR:PATH=%{_libdir} \
        -DSYSCONF_INSTALL_DIR:PATH=%{_sysconfdir} \
        -DSHARE_INSTALL_PREFIX:PATH=%{_datadir} \
        -DCMAKE_INSTALL_LIBDIR:PATH=%{_libdir} \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_C_FLAGS="${CFLAGS:-%optflags} -DNDEBUG" \
        -DCMAKE_CXX_FLAGS="${CXXFLAGS:-%optflags} -DNDEBUG" \
        -DCMAKE_EXE_LINKER_FLAGS="-Wl,--as-needed -Wl,--no-undefined -Wl,-z,now" \
        -DCMAKE_MODULE_LINKER_FLAGS="-Wl,--as-needed -Wl,--no-undefined -Wl,-z,now" \
        -DCMAKE_SHARED_LINKER_FLAGS="-Wl,--as-needed -Wl,--no-undefined -Wl,-z,now" \
%if "%{?_lib}" == "lib64"
        -DLIB_SUFFIX=64 \
%endif
        -DCMAKE_SKIP_RPATH:BOOL=ON \
        -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
        -DBUILD_SHARED_LIBS:BOOL=ON \
        -DBUILD_STATIC_LIBS:BOOL=OFF \
        -DCMAKE_COLOR_MAKEFILE:BOOL=OFF \
        -DCMAKE_INSTALL_DO_STRIP:BOOL=OFF \
        -DCMAKE_MODULES_INSTALL_DIR=%{_datadir}/cmake/Modules

make %{?_smp_mflags}

%install
make VERBOSE=1 DESTDIR=%{buildroot} install/fast -C %{builddir}

mkdir -p %{buildroot}%{_datadir}/%{name}/
install %{SOURCE1} %{buildroot}%{_datadir}/%{name}/


%if 0%{?suse_version}
%suse_update_desktop_file -i anmp
%endif

%check
cd %{builddir}
export CTEST_OUTPUT_ON_FAILURE=1
export LD_LIBRARY_PATH=%{buildroot}/%{_libdir}/:$LD_LIBRARY_PATH
%if !%{defined fedora}
  make check
%endif


%files
%defattr(-,root,root)
%{_bindir}/anmp-qt
%{_libdir}/libanmp.so
%dir %{_datadir}/%{name}/
%{_datadir}/%{name}/%{sffile}
%if 0%{?suse_version}
%{_datadir}/applications/anmp.desktop
%endif

%files progs
%defattr(-,root,root)
%{_bindir}/anmp-normalize
%{_bindir}/anmp-dump

