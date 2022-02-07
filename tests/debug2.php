<?php

$data = array();

$msgCount = array();

if ($_SERVER['argc'] == 2) {
    $fp = fopen($_SERVER['argv'][1], "r");
} else {
    $fp = fsockopen("hottub", 7777, $errno, $errstr, 30);
}
$fh = fopen("dump", "w");
$i = 0;
if (!$fp) {
    echo "$errstr ($errno)<br />\n";
    die();
} else {

    $buffer = "";
    $append = false;
    while (!feof($fp)) {
        $d = fgets($fp, 2);
        if ($d == hex2bin('FA') || $d == hex2bin('AE')) {
            $length = strlen($buffer);

            if (!isset($msgCount[$buffer])) {
                $msgCount[$buffer] = 0;
            }
            $msgCount[$buffer]++;

            fputs($fh, $buffer . "\n");

	    $hex = bin2hex($buffer);
	    // fa1433343043 = header + 340C = 34.0 deg C
	    if(preg_match("/fa14(.{6})43(.+)/", $hex, $matches)) {
		    $hex = $matches[2];
#		    print "hex = $hex\n";
                    $data['temp'] = substr($buffer, 2, 3)/10;
		    $light = substr($hex, 10, 2);
		    if($light == "08") {
			    $data['light'] = "on";
		    }
       		     elseif($light == "00") {
			    $data['light'] = "off";
		     }
       		     elseif($light == "80") {
			    $data['light'] = "on"; // but when pump 1 and pump 2 both on
		     }
		     else {
			    $data['light'] = "unknown";
			     print "light unknown = $light\n";
		     }

		    $pump = substr($hex, 0, 2);
		    if($pump == "00") {
			    $data['pump1'] = "off";
			    $data['pump2'] = "off";
		    }
		    elseif($pump == "02") {
			    $data['pump1'] = "on";
			    $data['pump2'] = "off";
		    }
		    elseif($pump == "03") {
			    $data['pump1'] = "on";
			    $data['pump2'] = "on";
		    }
		    elseif($pump == "08") {
			    $data['pump1'] = "off";
			    $data['pump2'] = "on";
		    }
		     else {
			    $data['pump1'] = "unknown";
			    $data['pump2'] = "unknown";
			     print "pump unknown = $pump\n";
		     }
	}

            $buffer = "";
            $append = true;
            $i++;

            if ($i % 100 == 0) {
                print_r($data);
            }
        }
        if ($append && $d != "\n") {
            $buffer .= $d;
        }
    }
    fclose($fp);
}
print_r($msgCount);

arsort($msgCount);

print_r($data);

$fh2 = fopen("msg-count", "w");
foreach ($msgCount as $k => $v) {
    fputs($fh2, "$v  = $k\n");
}
fclose($fh2);
