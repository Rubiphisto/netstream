Name:		netstream
Version:	1.1.0
Release:	1%{?dist}
Summary:	netstream is a net communication library which exploit the libevent's functionality

Group:		Development/Library
License:	GPL
Source0:	%{name}-%{version}.tar.gz

BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root
BuildArch:	x86_64

%description
netstream is a net communication library which exploit the libevent's functionality

%prep
%setup -q -n %{name}-%{version}


%build
%cmake .
make %{?_smp_mflags}


%install
make install DESTDIR=%{buildroot}


%files
%defattr(-,root,root)
%{_libdir}/*
%{_includedir}/*



%changelog

