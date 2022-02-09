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

            fputs($fh, $buffer . "\n");

	    $hex = bin2hex($buffer);

            if (!isset($msgCount[$hex])) {
                $msgCount[$hex] = 0;
            }
            $msgCount[$hex]++;

//print "hex = $hex\n";
	    // fa1433343043 = header + 340C = 34.0 deg C
	    if(preg_match("/fa14(.{8})(.+)/", $hex, $matches)) {
		    print "S ";
		    $hexTemp = $matches[1];
		    $hex = $matches[2];
		    if(strlen($hex) > 35) {
			    print "\nstatus hex = $hex\n";
	    	}
		    
		    if($hexTemp == "2d2d2d") {
			$data['temp'] = "----";
		    }
		    else{
	            #        $data['temp'] = substr($buffer, 2, 4);
		            $data['temp'] = substr($buffer, 2, 3)/10;
		    }

		    $pump = substr($hex, 1, 1);
		    if($pump == "0") {
			    $data['pump1'] = "off";
			    $data['pump2'] = "off";
		    }
		    elseif($pump == "2") {
			    $data['pump1'] = "on";
			    $data['pump2'] = "off";
		    }
		    elseif($pump == "3") { // incorrect
			 //     [heater] => unknown(6)
			//	    [light] => unknown(9)
			    $data['pump1'] = "on";
			    $data['pump2'] = "on";
		    }
		    elseif($pump == "8") {
			    $data['pump1'] = "off";
			    $data['pump2'] = "on";
		    }
		    else {
			    $data['pump1'] = "unknown ($pump)";
			    $data['pump2'] = "unknown ($pump)";
		     }

		    $heater = substr($hex, 2, 1);
       		     if($heater == "0") {
			     $data['heater'] = "off";

		     }
       		     elseif($heater == "1") {
			     $data['heater'] = "on";

		     }
       		     elseif($light == "2") {
			     $data['heater'] = "pluse";
		     }
		     else {
			    $data['heater'] = "unknown($heater)";
		     }

		    $light = substr($hex, 3, 1);
		    if($light == "3") {
			    $data['light'] = "on";
		    }
       		     elseif($light == "0") {
			    $data['light'] = "off";
		     }
		     else {
			    $data['light'] = "unknown($light)";
		     }

		     $status = substr($hex, 10, 2);
		     if($status == "00") {
			    $data['status'] = "";
		     }
		     elseif($status == "04") {
			    $data['status'] = "blower on";
		     }
		     else {
			    $data['status'] = "unknown ($status)";
		     }
		    // needs work, not sure any of this is right bar blower
		    /*
       		     if($status == "80") {
			    $data['status'] = "p1=on, p2=on";
		     }
		     elseif($status == "08") {
			    $data['status'] = "light on";
		     }
		     elseif($status == "80") {
			    $data['status'] = "p1=on, p2=on";
		     }
		     */

	    }
	    elseif(preg_match("/ae0d(.+)/", $hex, $matches)) {
		    $hex = $matches[1];
		    	print "U" . substr($hex, 0, 2) . " ";
		    	if(strlen($hex) > 29) {
		    		print "\nU " . $hex. "\n";
			}
	    }
	    else {
		    	print "unknown hex = $hex\n";
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
