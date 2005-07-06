<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
	<head>
		<meta http-equiv="Content-Type" content="text/html; charset=windows-1251">
		<title>map list</title>
		<style type="text/css">
			body {
				text-align: center
			}
			td {
				padding: 2px 6px
			}
			.even {
				background-color: #EEE
			}
			.odd {
				background-color: #DDD
			}
			.column1 {
				text-align: right
			}
			.column2 {
				text-align: center
			}
			.column3 {
				text-align: right
			}
		</style>
	</head>
		<body>
			<form enctype="multipart/form-data" action="map_erase.php" method="post">
				<p>delete map number <input type="text" name="map_index" size="2" /> <input type="submit" value=">" /></p>
			</form>
			<table cellspacing="0" cellpadding="0">
				<col class="column1"><col class="column2"><col class="column3">
<?

// definitions
define('MAP_LIST_PATH', '../map_list.dat');
define('FAILURE_MSG',   'failure');

// read the map list
$map_list = file_get_contents(MAP_LIST_PATH);
$map_list = unserialize($map_list);

// output map list
if (empty($map_list))
	echo '<tr><td>the map list is empty</td></tr>';
else
{
	$map_index = 1;
	foreach ($map_list as $map)
	{
		$map_name = $map['name'];
		$map_checksum = strtoupper($map['checksum']);
		if ($map_index % 2 == 0)
			echo "<tr class=\"even\"><td>{$map_index}.</td><td>{$map_name}</td><td>{$map_checksum}</td></tr>\n";
		else
			echo "<tr class=\"odd\"><td>{$map_index}.</td><td>{$map_name}</td><td>{$map_checksum}</td></tr>\n";
		++$map_index;
	}
}

?>
		</table>
	</body>
</html>