#!/usr/bin/perl -w
# -*- tab-width: 4; mode: perl; -*-
#
# A hack to convert a file of NMEA GGA and RMC records to a GPX file.
#
#

use strict;

my($line,$csum,$len,$rest);
my(@gga,@rmc,@gsa);
my($raw_time,$raw_date,$raw_lat,$raw_long);
my($hdop,$hdop2,$vdop,$fix) = (99.99,99.99,99.99,0);
my($zulu,$hours,$mins,$secs,$dsecs,$daysecs,$start,$elapsed,$offset);
my($latitude,$ns,$longitude,$ew,$date,$time,$alt_msl,$speed_kn,$heading,$sats) = (0.0,'N',0.0,'E','','',0,0,0,0);
my($max_kn,$max_alt_msl,$base_alt,$speed_ms) = (0,0,0,0);
my($lines,$max_line_length,$longest_line) = (0,0,'');
my($GPX,$TSV);

my $gpx_file = 'gps_log.gpx';
my $tsv_file = 'gps_log.tsv';
my $encoding = ':encoding(UTF-8)';

#

print $^O . "\n";

#
# Windows perls (particularly the ActiveState one) seem to want the three parameter
# version of open();
#

if ($^O eq 'MSWin32') {

    open($GPX,"> $encoding",$gpx_file) || die "Cannot open GPX file: $!";
    open($TSV,"> $encoding",$tsv_file) || die "Cannot open tsv file: $!";

} else { # 'linux', 'darwin' etc.

    open($GPX,"> $gpx_file") || die "Cannot open GPX file: $!";
    open($TSV,"> $tsv_file") || die "Cannot open tsv file: $!";
}

print $TSV "zulu\telapsed\taltitude\tsats\thdop\tvdop\tm/s\n";

#

print $GPX <<EndOfHeader;
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
            
            $time = sprintf("%02d:%02d:%02d.%d0",$hours,$mins,$secs,$dsecs);

            if (!$start) {
                $start = $daysecs;
            }

            $elapsed = 0.1 * int(10 * (0.00001 + $daysecs - $start - $offset));
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

            print STDERR "Assuming that ground is ${base_alt} m MSL.\n";
        }
        
        if ($alt_msl > $max_alt_msl) {

            $max_alt_msl = $alt_msl;
        }

        if (($date)&&($time)) {

            $zulu = "${date}T${time}Z";
            
            print $GPX sprintf("      <trkpt lat=\"%.6f\" lon=\"%.6f\">",$latitude,$longitude);
            print $GPX "<time>$zulu</time><ele>${alt_msl}</ele><speed>${speed_ms}</speed><course>${heading}</course><sat>${sats}</sat></trkpt>\n";

            print $TSV "${zulu}\t${elapsed}\t${alt_msl}\t${sats}\t${hdop}\t${vdop}\t${speed_ms}\n";
        }
    }

    if ($line =~ /RMC/) {

        @rmc = split(',',$line);

        $speed_kn = $rmc[7];
        $heading  = $rmc[8];
        $raw_date = $rmc[9];
    
        if ($raw_date =~ /^(\d{2})(\d{2})(\d{2})/) {

            $date = $3 + 2000 . "-${2}-${1}";
        }

        $speed_ms = sprintf("%.1f",$speed_kn * 0.514444);

        if ($speed_kn > $max_kn) {

            $max_kn = $speed_kn;
        }
    }

    if ($line =~ /GSA/) {

        @gsa  = split(',',$line);
        $hdop = $gsa[16];
        $vdop = $gsa[17];
    }
}

#

print $GPX <<EndOfFooter;
    </trkseg>
  </trk>
</gpx>
EndOfFooter

print STDERR "\n${lines} lines read, max line length was ${max_line_length}\n${longest_line}\n";

my $mph    = int($max_kn      * 1.15);
my $kph    = int($max_kn      * 1.85);
my $alt_ft = int($max_alt_msl * 3.28084);

print STDERR sprintf("%3d knots (%d mph)\n",$max_kn,$mph);
print STDERR sprintf("%3d m MSL (%d ft, %d m AGL)\n",
                     $max_alt_msl,$alt_ft,$max_alt_msl - $base_alt);

close $TSV;
close $GPX;

__END__
