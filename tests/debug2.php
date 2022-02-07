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

            if ($length == 23) {
                $data['temp'] = substr($buffer, 2, 4);
            } else {
#                print "length = " . $length . "\thex= " . bin2hex($buffer) . "\n";
##                var_dump($buffer);
#                print "\n";
            }
            if (!isset($msgCount[$buffer])) {
                $msgCount[$buffer] = 0;
            }
            $msgCount[$buffer]++;

            fputs($fh, $buffer . "\n");

	    $hex = bin2hex($buffer);

	    if(preg_match("/fa1433333043(.+)/", $hex, $matches)) {
		    $hex = $matches[1];
		  //  print "hex = $hex\n";
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
