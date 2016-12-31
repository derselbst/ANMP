
# clang <= 3.7 doesnt build with build-id
%if %{defined fedora}
%undefine _missing_build_ids_terminate_build
%endif

%define soname 0

Name: anmp
Version: 2
Release: 0
License: GPL-2.0
Summary: Another Nameless Music Player
Url: https://www.github.com/derselbst/ANMP
Group: Development/Libraries/C and C++
Source0: anmp-%{version}.tar.bz2

BuildRoot: %{_tmppath}/%{name}-%{version}-build

Requires: libanmp%{soname}


%ifarch x86_64
BuildRequires: clang >= 3.5
%else
BuildRequires: gcc-c++ >= 4.8
%endif


%if 0%{?mageia}
BuildRequires: cmake >= 1:3.1.0
%else
BuildRequires: cmake >= 3.1.0
%endif

BuildRequires: pkgconfig(fluidsynth)
BuildRequires: pkgconfig(smf)
BuildRequires: libcue-devel
BuildRequires: libgme-devel
BuildRequires: libmad-devel pkgconfig(id3tag)
BuildRequires: pkgconfig(sndfile)
BuildRequires: vgmstream-devel
%ifarch x86_64 i586 i686
BuildRequires: lazyusf2-devel
%endif

# SuSE specific:
%if 0%{?suse_version} >= 1320
BuildRequires: libavcodec-devel
BuildRequires: libavformat-devel
BuildRequires: libavutil-devel
BuildRequires: libswresample-devel
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
BuildRequires: pkgconfig(Qt5OpenGL)


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

# clang fails linking the stack guard on ppc64 and has problems with std::atomic on i586
# but clang is cool, so use it on x86_64, else fallback to gcc
# update 2016-12-31: when trying to build c++14 code with libstdc++-4.8, clang complains: "no member named 'gets' in the global namespace"; so fallback to gcc :(
# %%ifarch x86_64
# %%cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
# %%else
%cmake
# %%endif

make %{?_smp_mflags}

%install

%if %{defined suse_version}
%__make VERBOSE=1 DESTDIR=%{buildroot} install/fast -C %__builddir
%else
%cmake_install
%endif

mv %{buildroot}/%{_libdir}/libanmp.so %{buildroot}/%{_libdir}/libanmp.so.%{soname}

ln -s /%{_libdir}/libanmp.so.%{soname} %{buildroot}/%{_libdir}/libanmp.so

ln -s /%{_bindir}/anmp-qt %{buildroot}/%{_bindir}/anmp

%check
export CTEST_OUTPUT_ON_FAILURE=1
%__make test

%post -n libanmp%{soname} -p /sbin/ldconfig

%postun -n libanmp%{soname} -p /sbin/ldconfig


%files
%defattr(-,root,root)
%{_bindir}/anmp
%{_bindir}/anmp-qt

%files devel
%defattr(-,root,root)
%{_libdir}/libanmp.so
%{_includedir}/anmp/

%files -n libanmp%{soname}
%defattr(-,root,root)
%{_libdir}/libanmp.so.*

%files progs
%defattr(-,root,root)
%{_bindir}/anmp-normalize


