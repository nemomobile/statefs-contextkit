Summary: Statefs-Contextkit subscriber adapter for Qt5
Name: statefs-contextkit-subscriber
Version: x.x.x
Release: 1
License: LGPLv2
Group: System Environment/Tools
URL: http://github.com/nemomobile/statefs-contextkit
Source0: %{name}-%{version}.tar.bz2
BuildRequires: pkgconfig(statefs) >= 0.2.11
BuildRequires: cmake >= 2.8
BuildRequires: pkgconfig(cor) >= 0.1.3
BuildRequires: pkgconfig(Qt5Core)
Requires: statefs

%description
Provides adapter to use Contextkit API to access statefs

%package devel
Summary: Contextkit property interface using statefs
Group: System Environment/Libraries
Requires: statefs-contextkit-subscriber
%description devel
Contextkit property interface using statefs instead of contextkit

%prep
%setup -q

%build
%cmake -DUSEQT=5
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_libdir}/libcontextkit-statefs.so

%files devel
%defattr(-,root,root,-)
%{_includedir}/contextproperty.h
%{_libdir}/pkgconfig/contextkit-statefs.pc

