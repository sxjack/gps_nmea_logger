#!/usr/bin/perl -w
# -*- tab-width: 4; mode: perl; -*-
#
# A hack to convert a file of NMEA GGA and RMC records to a GFX file.
#
#

use strict;

my($line,$len,$rest);
my($gga,@rmc);
my($raw_time,$raw_date,$raw_lat,$raw_long);
my($hdop,$fix);
my($latitude,$ns,$longitude,$ew,$date,$time,$alt_msl,$speed,$heading,$sats) = (0.0,'N',0.0,'E','','',0,0,0,0);
my($lines,$max_line_length,$longest_line) = (0,0,'');

#

print <<EndOfHeader;
<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<gpx 
  version="1.0"
  creator="GPS NMEA Parser"
  author="Steve Jack"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
  xmlns="http://www.topografix.com/GPX/1/1" 
  xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd" >
  <name>GPS Track</name>
  <trk>
    <name>GPS Track</name>
    <trkseg>
EndOfHeader

#
    
while (<>) {

    $line = $_;

    ++$lines;

    if (($len = length($line)) > $max_line_length) {

        $max_line_length = $len;
        $longest_line    = $line;
    }
    
    if ($line =~ /GGA/) {

        ($gga,$raw_time,$raw_lat,$ns,$raw_long,$ew,$fix,$sats,$hdop,$alt_msl,$rest) = split(',',$line); 

        if ($raw_time =~ /^(\d{2})(\d{2})(\d{2})\.(\d)/) {

            $time = "${1}:${2}:${3}.$4";
        }

        if ($raw_lat =~ /^(\d{2})(.*)$/) {

            $latitude = $1 + ($2 / 60);

            if ($ns eq 'S') {
                $latitude *= -1;
            }
        }

        if ($raw_long =~ /^(\d{3})(.*)$/) {

            $longitude = $1 + ($2 / 60);

            if ($ew eq 'W') {
                $longitude *= -1;
            }
        }

        if (($date)&&($time)) {
        
            print sprintf("      <trkpt lat=\"%.6f\" lon=\"%.6f\">",$latitude,$longitude);
            print "<time>${date}T${time}Z</time><ele>${alt_msl}</ele><speed>${speed}</speed><course>${heading}</course><sat>${sats}</sat></trkpt>\n";

        }
    }

    if ($line =~ /RMC/) {

        @rmc      = split(',',$line);
        $speed    = $rmc[7];
        $heading  = $rmc[8];
        $raw_date = $rmc[9];
    
        if ($raw_date =~ /^(\d{2})(\d{2})(\d{2})/) {

            $date = $3 + 2000 . "-${2}-${1}";
        }
    }
}

#

print <<EndOfFooter;
    </trkseg>
  </trk>
</gpx>
EndOfFooter

print STDERR "${lines} lines read, max line length was ${max_line_length}\n${longest_line}";

__END__
