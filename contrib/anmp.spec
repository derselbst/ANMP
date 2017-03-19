#
# spec file for package sonic-visualiser
#
# Copyright (c) 2016-2017 Tom Mbrt <tom.mbrt@googlemail.com>
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.


# # clang <= 3.7 doesnt build with build-id
# %if %{defined fedora}
# %undefine _missing_build_ids_terminate_build
# %endif


%define soname 0
%define builddir build
%define sffile OldSeiterPiano.sf2

%bcond_with sndfile
%bcond_with aopsf
%bcond_with mad
%bcond_with gme
%bcond_with modplug
%bcond_with vgmstream
%bcond_with midi
# SuSE specific:
# everything newer openSUSE 13.2 or openSUSE Leap
%if 0%{?suse_version} >= 1320 || 0%{?suse_version} == 1315
%bcond_with ffmpeg
%endif

%ifarch x86_64 i586 i686
%bcond_with lazyusf
%endif

%bcond_with alsa
%bcond_with jack
%bcond_with portaudio
%bcond_with ebur128

%bcond_with cue
%bcond_with qt
%bcond_with visualizer


Name: anmp
Version: 3
Release: 0
License: GPL-2.0
Summary: Another Nameless Music Player
Url: https://www.github.com/derselbst/ANMP
Group: Development/Libraries/C and C++
Source0: anmp-%{version}.tar.bz2
Source1: %{sffile}
BuildRoot: %{_tmppath}/%{name}-%{version}-build

Requires: libanmp%{soname} = %{version}


BuildRequires: gcc-c++ >= 4.8

%if 0%{?mageia}
BuildRequires: cmake >= 1:3.1.0
%else
BuildRequires: cmake >= 3.1.0
%endif

# %if 0%{?suse_version} == 1320
# %ifarch x86_64
# BuildRequires: clang >= 3.5
# %endif
# %endif

%if 0%{with ffmpeg}
BuildRequires: libavcodec-devel
BuildRequires: libavformat-devel
BuildRequires: libavutil-devel
BuildRequires: libswresample-devel
%endif

%if 0%{with midi}
BuildRequires: pkgconfig(fluidsynth)
BuildRequires: pkgconfig(smf)
%endif

%if 0%{with gme}
BuildRequires: libgme-devel
%endif

%if 0%{with mad}
BuildRequires: libmad-devel
BuildRequires: pkgconfig(id3tag)
%endif

%if 0%{with sndfile}
BuildRequires: pkgconfig(sndfile)
%endif

%if 0%{with modplug}
BuildRequires: libmodplug-devel
%endif

%if 0%{with vgmstream}
BuildRequires: vgmstream-devel
%endif

%if 0%{with lazyusf}
BuildRequires: lazyusf2-devel
%endif


%if 0%{with alsa}
BuildRequires: pkgconfig(alsa)
%endif

%if 0%{with jack}
BuildRequires: pkgconfig(jack)
BuildRequires: pkgconfig(samplerate)
%endif

%if 0%{with portaudio}
BuildRequires: pkgconfig(portaudio-2.0)
%endif

%if 0%{with ebur128}
BuildRequires: libebur128-devel
%endif


%if 0%{with qt}
BuildRequires: pkgconfig(Qt5Widgets)
BuildRequires: pkgconfig(Qt5DBus)
%endif

%if 0%{with visualizer}
BuildRequires: pkgconfig(Qt5OpenGL)
%endif

%if 0%{with cue}
BuildRequires: libcue-devel
%endif


%description
A MusicPlayer capable of playing various formats.

The key features are:
  - gapless playback
  - cue sheets
  - arbitrary (forward) looping of songs
  - easy attempt to implement new formats



%package -n libanmp%{soname}
Summary:        core lib for %{name}
Group:          Development/Libraries/C and C++

%description -n libanmp%{soname}
Library providing basic functionality for %{name}


%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries/C and C++
Requires:       libanmp%{soname} = %{version}

%description    devel
Development files for %{name}


%package        progs
Summary:        Programs for %{name}
Group:          Development/Libraries/C and C++
Requires:       libanmp%{soname} = %{version}

%description    progs
Additional useful tools for %{name}


%prep
%setup -q

%build
mkdir -p %{builddir}
cd %{builddir}

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

make %{?_smp_mflags} anmp-qt

%install
make VERBOSE=1 DESTDIR=%{buildroot} install/fast -C %{builddir}

mv %{buildroot}/%{_libdir}/libanmp.so %{buildroot}/%{_libdir}/libanmp.so.%{soname}
ln -s /%{_libdir}/libanmp.so.%{soname} %{buildroot}/%{_libdir}/libanmp.so
ln -s /%{_bindir}/anmp-qt %{buildroot}/%{_bindir}/anmp

mkdir -p %{buildroot}%{_datadir}/%{name}/
install %{SOURCE1} %{buildroot}%{_datadir}/%{name}/

%check
cd %{builddir}
export CTEST_OUTPUT_ON_FAILURE=1
export LD_LIBRARY_PATH=%{buildroot}/%{_libdir}/:$LD_LIBRARY_PATH
%if !%{defined fedora}
  make check
%endif

%post -n libanmp%{soname} -p /sbin/ldconfig

%postun -n libanmp%{soname} -p /sbin/ldconfig


%files
%defattr(-,root,root)
%{_bindir}/anmp
%{_bindir}/anmp-qt
%dir %{_datadir}/%{name}/
%{_datadir}/%{name}/%{sffile}

%files devel
%defattr(-,root,root)
%{_libdir}/libanmp.so
%{_includedir}/anmp/

%files -n libanmp%{soname}
%defattr(-,root,root)
%{_libdir}/libanmp.so.*

%files progs
%defattr(-,root,root)
# %%{_bindir}/anmp-normalize


