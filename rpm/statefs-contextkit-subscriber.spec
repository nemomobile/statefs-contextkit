Summary: Statefs-Contextkit subscriber adapter for Qt5
Name: statefs-contextkit-subscriber
Version: x.x.x
Release: 1
License: LGPLv2
Group: System Environment/Tools
URL: http://github.com/nemomobile/statefs-contextkit
Source0: %{name}-%{version}.tar.bz2
BuildRequires: pkgconfig(statefs) >= 0.3.6
BuildRequires: cmake >= 2.8
BuildRequires: pkgconfig(cor) >= 0.1.5
BuildRequires: pkgconfig(Qt5Core)
Requires: statefs

%description
Provides adapter to use Contextkit API to access statefs

%package -n statefs-qt5
Summary: StateFS Qt5 bindings
Group: System Environment/Libraries
Requires: statefs >= 0.3.6
%description -n statefs-qt5
%{summary}

%package -n statefs-qt5-devel
Summary: StateFS Qt5 bindings development files
Group: Development/Libraries
Requires: statefs-qt5 = %{version}
%description -n statefs-qt5-devel
%{summary}

%package devel
Summary: Contextkit property interface using statefs
Group: System Environment/Libraries
Requires: statefs-contextkit-subscriber
%description devel
Contextkit property interface using statefs instead of contextkit

%package -n statefs-qt-doc
Summary: StateFS Qt bindings documentation
Group: Documenation
BuildRequires: doxygen
%if 0%{?_with_docs:1}
BuildRequires: graphviz
%endif
%description -n statefs-qt-doc
%{summary}


%prep
%setup -q

%build
%cmake -DUSEQT=5 -DSTATEFS_QT_VERSION=%{version}
make %{?jobs:-j%jobs}
make doc

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

install -d -D -p -m755 %{buildroot}%{_datarootdir}/doc/statefs-qt/html
cp -R doc/html/ %{buildroot}%{_datarootdir}/doc/statefs-qt/


%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_libdir}/libcontextkit-statefs.so

%files -n statefs-qt5
%defattr(-,root,root,-)
%{_libdir}/libstatefs-qt5.so

%files -n statefs-qt5-devel
%defattr(-,root,root,-)
%{_qt5_headerdir}/statefs/qt/*.hpp
%{_libdir}/pkgconfig/statefs-qt5.pc

%files devel
%defattr(-,root,root,-)
%{_includedir}/contextproperty.h
%{_libdir}/pkgconfig/contextkit-statefs.pc

%files -n statefs-qt-doc
%defattr(-,root,root,-)
%{_datarootdir}/doc/statefs-qt/html/*

