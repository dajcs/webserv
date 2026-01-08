#!/usr/bin/perl
use strict;
use warnings;

print "Content-Type: text/plain\n\n";
print "Perl CGI \"Hello World!\"\n";
print "REQUEST_METHOD = $ENV{REQUEST_METHOD}\n";
print "QUERY_STRING = $ENV{QUERY_STRING}\n";

