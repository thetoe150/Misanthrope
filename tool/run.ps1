# Set the path to your executable (adjust if needed)
$exePath = "D:\PP\Misanthrope\tool\build\bin\Debug\Exporter.exe"

$vertexPath   = "D:\PP\Misanthrope\data\shaders\candles.vert.spv"
$fragmentPath = "D:\PP\Misanthrope\data\shaders\candles.frag.spv"

$gltfPath   = "D:\PP\Misanthrope\data\models\candles\scene.gltf"
$binaryPath = "D:\PP\Misanthrope\data\models\candles\scene.bin"

& $exePath $vertexPath $fragmentPath $gltfPath $binaryPath
# & $exePath $gltfPath $binaryPath

# Combine the arguments into a single string
# $arguments = $vertexPath $fragmentPath

# Run the executable with the arguments
# Start-Process -FilePath $exePath -ArgumentList $arguments -NoNewWindow -Wait
