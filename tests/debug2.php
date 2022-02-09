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
	    if(preg_match("/fa14(.{8})(.+)/", $hex, $matches)) {
		    $hexTemp = $matches[1];
		    $hex = $matches[2];
		    #print "hex = $hex\n";
		    
		    #                    $data['temp'] = substr($buffer, 2, 3)/10;
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
		    if($light == "03") {
			    $data['light'] = "on";
		    }
       		     elseif($light == "00") {
			    $data['light'] = "off";
		     }
		     else {
			    $data['light'] = "unknown($light)";
		     }


		     $status = substr($hex, 10, 2);
       		     if($status == "80") {
			    $data['status'] = "p1=on, p2=on";
		     }
		     elseif($status == "04") {
			    $data['status'] = "blower on";
		     }
		     elseif($status == "08") {
			    $data['status'] = "light on";
		     }
		     elseif($status == "80") {
			    $data['status'] = "p1=on, p2=on";
		     }
		     elseif($status == "00") {
			    $data['status'] = "";
		     }
		     else {
			    $data['status'] = "unknown";
			     print "status unknown = $status\n";
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
