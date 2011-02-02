Summary: Miscellaneous lustre tools from LLNL
Name: See META file
Version: See META file
Release: See META file
License: GPL
Group: System Environment/Base
#URL: 
Packager: Christopher J. Morrone <morrone2@llnl.gov>
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: lustre
Requires: lustre

%description

%prep
%setup -q

%build
%configure
make

%install
rm -rf $RPM_BUILD_ROOT
DESTDIR="$RPM_BUILD_ROOT" make install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc
%{_sbindir}/*
%{_mandir}/*/*

%changelog
* Mon Dec  6 2010 Christopher J. Morrone <morrone@butters.llnl.gov> - tools-llnl
- Initial build.

