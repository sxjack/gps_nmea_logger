#!/usr/bin/perl -w
# -*- tab-width: 4; mode: perl; -*-
#
# A hack to convert a file of NMEA GGA and RMC records to a GFX file.
#
#

use strict;

my($line,$csum,$len,$rest);
my(@gga,@rmc,@gsa);
my($raw_time,$raw_date,$raw_lat,$raw_long);
my($hdop,$hdop2,$vdop,$fix) = (99.99,99.99,99.99,0);
my($zulu,$hours,$mins,$secs,$dsecs,$daysecs,$start,$elapsed,$offset);
my($latitude,$ns,$longitude,$ew,$date,$time,$alt_msl,$speed,$heading,$sats) = (0.0,'N',0.0,'E','','',0,0,0,0);
my($max_speed,$max_alt_msl,$base_alt) = (0,0,0);
my($lines,$max_line_length,$longest_line) = (0,0,'');

#

open(GPX,"> gps_log.gpx") || die "Cannot open GPX file: $!";

open(TSV,"> gps_log.tsv") || die "Cannot open tsv file: $!";

print TSV "zulu\telapsed\taltitude\tsats\thdop\tvdop\n";

#

print GPX <<EndOfHeader;
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

$start = $elapsed = $offset = 0;

# $offset = 55;

while (<>) {

    ++$lines;

    if (($len = length($_)) > $max_line_length) {

        $max_line_length = $len;
        $longest_line    = $_;
    }
    
    if (m/\$(.*)\*/) {

        $line = $1;

    } else {

        next;
    }

    if ($line =~ /GGA/) {

        @gga = split(',',$line);

         $raw_time = $gga[1];
         $raw_lat  = $gga[2];
         $ns       = $gga[3];
         $raw_long = $gga[4];
         $ew       = $gga[5];
         $fix      = $gga[6];
         $sats     = $gga[7];
         $hdop2    = $gga[8];
         $alt_msl  = $gga[9];
        
        if ($raw_time =~ /^(\d{2})(\d{2})(\d{2})\.(\d)/) {

            $hours   = $1;
            $mins    = $2;
            $secs    = $3;
            $dsecs   = $4;
            $daysecs = ($hours * 3600) + ($mins * 60) + $secs + (0.1 * $dsecs); 
            
            $time  = "${hours}:${mins}:${secs}.${dsecs}";

            if (!$start) {
                $start = $daysecs;
            }

            $elapsed = $daysecs - $start - $offset;
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

        if (($base_alt == 0)&&($sats > 7)) {

            $base_alt = $alt_msl;
        }
        
        if ($alt_msl > $max_alt_msl) {

            $max_alt_msl = $alt_msl;
        }

        if (($date)&&($time)) {

            $zulu = "${date}T${time}Z";
            
            print GPX sprintf("      <trkpt lat=\"%.6f\" lon=\"%.6f\">",$latitude,$longitude);
            print GPX "<time>$zulu</time><ele>${alt_msl}</ele><speed>${speed}</speed><course>${heading}</course><sat>${sats}</sat></trkpt>\n";

            print TSV "${zulu}\t${elapsed}\t${alt_msl}\t${sats}\t${hdop}\t${vdop}\n";
        }
    }

    if ($line =~ /RMC/) {

        @rmc = split(',',$line);

        $speed    = $rmc[7];
        $heading  = $rmc[8];
        $raw_date = $rmc[9];
    
        if ($raw_date =~ /^(\d{2})(\d{2})(\d{2})/) {

            $date = $3 + 2000 . "-${2}-${1}";
        }

        if ($speed > $max_speed) {

            $max_speed = $speed;
        }
    }

    if ($line =~ /GSA/) {

        @gsa  = split(',',$line);
        $hdop = $gsa[16];
        $vdop = $gsa[17];
    }
}

#

print GPX <<EndOfFooter;
    </trkseg>
  </trk>
</gpx>
EndOfFooter

print STDERR "\n${lines} lines read, max line length was ${max_line_length}\n${longest_line}\n";

print STDERR sprintf("%3d",$max_speed) . " knots\n";
print STDERR sprintf("%3d m MSL (%d m AGL)\n",$max_alt_msl,$max_alt_msl - $base_alt);

close TSV;
close GPX;

__END__
