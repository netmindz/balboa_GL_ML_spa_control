<?php

$data = array();

$fp = fsockopen("hottub", 7777, $errno, $errstr, 30);
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


            if ($length == 64) {
                $data['temp'] = substr($buffer, 8, 4);
            } elseif ($length == 72) {
                $data['temp_target'] = substr($buffer, 2, 4);
            } else {
                print "length = " . $length . "\n\n";
                var_dump($buffer);
            }
            #		    print "temp: " . substr($buffer,8,4) . "\n";

            fputs($fh, $buffer . "\n");


            $buffer = "";
            $append = true;
            $i++;

            if ($i % 100 == 0) {
                print_r($data);
            }
        }
        if ($append) {
            $buffer .= $d;
        }
    }
    fclose($fp);
}
