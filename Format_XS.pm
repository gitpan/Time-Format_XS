=for gpg
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA1

=head1 NAME

Time::Format_XS - Companion module for Time::Format, to speed up time formatting.

=head1 VERSION

This document describes version 0.11 of Time::Format_XS, July 11, 2003.

Note: Time::Format_XS's version number is set to the Time::Format version
number that it is distributed with, so this number doesn't mean a whole lot.

=cut

use strict;
package Time::Format_XS;
use vars '$VERSION';
$VERSION = '0.11';

require XSLoader;
XSLoader::load('Time::Format_XS', $VERSION);
1;
__END__

=head1 SYNOPSIS

  Install this module, but do not use it.

=head1 DESCRIPTION

The Time::Format module (q.v.) is a handy and easy-to-use way of
formatting dates and times.  It's not particularly slow, but it's not
particularly speedy, either.

This module, Time::Format_XS, provides a huge performance improvement
for the main formatting function in Time::Format.  This is the
C<time_format> function, usually accessed via the C<%time> hash.  On
my test system, this function was 12 times faster with the
Time::Format_XS module installed.

To use this module, all you have to do is install it.  Versions 0.10
and later of Time::Format will automatically detect if your system has
Time::Format_XS installed and will use it without your having to
change any code of yours that uses Time::Format.

Time::Format_XS is distributed as a separate module because not
everybody can use XS.  Not everyone has a C compiler.  Also,
installations with a statically-linked perl may not want to recompile
their perl binary just for this module.  Rather than render
Time::Format useless for these people, the XS portion was put into a
separate module.

Programs that you write do not need to know whether Time::Format_XS is
installed or not.  They should just "use Time::Format" and let
Time::Format worry about whether or not it can use XS.  If the
Time::Format_XS is present, Time::Format will be faster.  If not, it
won't.  Either way, it will still work, and your code will not have to
change.

=head2 EXPORTS

None.

=head1 SEE ALSO

 Time::Format

=head1 AUTHOR / COPYRIGHT

Eric Roode, roode@cpan.org

Copyright (c) 2003 by Eric J. Roode. All Rights Reserved.  This module
is free software; you can redistribute it and/or modify it under the
same terms as Perl itself.

=cut

=begin gpg

-----BEGIN PGP SIGNATURE-----
Version: GnuPG v1.2.2 (GNU/Linux)

iD8DBQE/DuCAY96i4h5M0egRAgctAJ4yylok2ta/tqv0ntJRnQEAWexSPwCg4YhC
H5cF0dgrgy4b/v+mRZjL1Qo=
=0VhX
-----END PGP SIGNATURE-----

=end gpg
