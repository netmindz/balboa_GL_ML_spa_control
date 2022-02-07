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
                print "length = " . $length . "\thex= " . bin2hex($buffer) . "\n";
                var_dump($buffer);
                print "\n";
            }
            if (!isset($msgCount[$buffer])) {
                $msgCount[$buffer] = 0;
            }
            $msgCount[$buffer]++;

            fputs($fh, $buffer . "\n");


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
