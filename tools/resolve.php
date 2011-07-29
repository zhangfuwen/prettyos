<?php
$name=$_GET["hostname"];

$name=htmlentities($name);

$result = gethostbyname($name);
echo $result;

?>