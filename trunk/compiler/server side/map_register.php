<?

// definitions
define('MAP_LIST_PATH',    'map_list.dat');
define('SUCCESS_MSG',      'success');
define('FAILURE_MSG',      'failure');
define('TAKEN_MSG',        'taken');
define('INVALID_NAME_MSG', 'invalid');

// check user agent
if ($HTTP_USER_AGENT != 'Perimeter Map Compiler')
	exit(FAILURE_MSG);

// validate variables
if (!isset($map_name) || !is_string($map_name))
	exit(FAILURE_MSG);
if (!isset($map_checksum) || !is_string($map_checksum))
	exit(FAILURE_MSG);

// make sure map_name has valid length
$map_name_length = strlen($map_name);
if ($map_name_length <= 0 || $map_name_length > 20)
	exit(INVALID_NAME_MSG);

// make sure map_name contains only valid characters
$valid_chars = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-\'\\';
for ($i = 0; $i != $map_name_length; ++$i)
	if (!strstr($valid_chars, $map_name{$i}))
		exit(INVALID_NAME_MSG);
$map_name = stripslashes($map_name);

// the name UNREGISTERED is reserved
if ('UNREGISTERED' == $map_name)
	exit(INVALID_NAME_MSG);

// read the map list
$map_list = file_get_contents(MAP_LIST_PATH);
if (!$map_list)
	exit(FAILURE_MSG);
$map_list = unserialize($map_list);
if (!$map_list)
	$map_list = array();

// check if the map name is taken
$map_name_taken = FALSE;
foreach ($map_list as $map)
{
	if ($map['name'] == $map_name)
	{
		$map_name_taken = TRUE;
		if ($map['checksum'] != $map_checksum)
			exit(TAKEN_MSG);
		break;
	}
}

// append map_name to the list
if (!$map_name_taken)
	array_push($map_list, array('name' => $map_name, 'checksum' => $map_checksum));

// write the map list
$file = fopen(MAP_LIST_PATH, 'w');
if ($file && fwrite($file, serialize($map_list)) && fclose($file))
	exit(SUCCESS_MSG);
else
	exit(FAILURE_MSG);

?>