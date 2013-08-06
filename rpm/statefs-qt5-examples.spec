Summary: Example applications using statefs Qt bindings
Name: statefs-qt5-examples
Version: x.x.x
Release: 1
License: LGPLv2
Group: Documentation
URL: http://github.com/nemomobile/statefs-contextkit
Source0: %{name}-%{version}.tar.bz2
BuildRequires: cmake >= 2.8
BuildRequires: pkgconfig(statefs-qt5) = %{version}
BuildRequires: pkgconfig(contextkit-statefs) = %{version}
BuildRequires: pkgconfig(Qt5Core)

%description
%{summary}

%prep
%setup -q

%build
cd examples/inout
%cmake
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
cd examples/inout
make install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_libdir}/statefs/examples/statefs-inout-example-reader
%{_libdir}/statefs/examples/statefs-inout-example-writer
%{_datarootdir}/statefs/examples/statefs-inout-example.conf

%post
%{_bindir}/statefs register --statefs-type=inout %{_datarootdir}/statefs/examples/statefs-inout-example.conf || :

