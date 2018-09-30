#!/usr/local/bin/perl -w

# This tool calculates probability over the tail of a binomial
# distribution.  The calculations is done directly, without using any
# approximations.
#
# This program is in the public domain.  It was written
# by Brett Kessler and David Gelbart.
#
# $Header: /home/srilm/devel/utils/src/RCS/cumbin.pl,v 1.1 2008/11/05 19:03:43 stolcke Exp $
#

use strict;
use POSIX;

if (@ARGV != 2) {
  die "Usage: $0 n k\n";
}
my $n = $ARGV[0];
my $k = $ARGV[1];
my $p = 0.5;

if (($n - $k) > $k) {
  die "Did you choose the right value of k?\n";
}
my $P = tailBinomial($n, $k, $p);
print "One-tailed: P(k >= ${k} | n=${n}, p=${p}) = ${P}\n";
$P = 2 * $P;
print "Two-tailed: 2*P(k >= ${k} | n=${n}, p=${p}) = ${P}\n";

# Calculate the sum over the tail of the binomial probability distribution.
sub tailBinomial {
  my($N, $k, $p) = @_;

  my $sum = 0;
  for (my $i = $k; $i <= $N; $i++) {
    $sum += exp(logBinomial($N, $i, $p));
  }
  $sum;
}

# We use logarithms during calculation to avoid overflow during the
# calculation of factorials and underflow during the calculation of
# powers of probabilities.  This function calculates the log of
# binomial probability for given N, k, p.
sub logBinomial {
  my($N, $k, $p) = @_;
  my $q = 1 - $p;

  # These safety checks were inspired by the code at
  # http://faculty.vassar.edu/lowry/binomialX.html
  die "Error: N not integer" if ($N != floor($N));
  die "Error: k not integer" if ($k != floor($k));
  die "Error: k > N" if ($k > $N);
  die "Error: p > 1" if ($p > 1);
  die "Error: N < 1" if ($N < 1);

  logBinomCoeff($N, $k) + $k * log($p) + ($N - $k) * log($q);
}

# Calculcate the log of the binomial coefficient for given N and k.
sub logBinomCoeff {
  my($N, $k) = @_;
  logFactorial($N) - logFactorial($k) - logFactorial($N - $k);
}

# Calculate the log of the factorial of the argument.
sub logFactorial {
  my($N) = @_;

  my $prod = 0;
  for (my $i = 2; $i <= $N; $i++) {
    $prod += log($i);
  }

  $prod;
}
