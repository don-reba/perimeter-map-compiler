<?

// make sure map_name has valid length
$map_name_length = strlen($map_name);
if ($map_name_length <= 0 || $map_name_length >= 20)
	echo 'long';
// make sure map_name contains only valid characters
$valid_chars = '0123456789abcdefghijklmnopqrstuvwxyz_-';
for ($i = 0; $i != $map_name_length; ++$i)
if (!strstr($valid_chars, $map_name{$i}))
	echo 'invalid';

?>