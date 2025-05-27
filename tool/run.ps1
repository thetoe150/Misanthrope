# Set the path to your executable (adjust if needed)
$currentPath = Get-Location

$exePath = Join-Path -Path $currentPath -ChildPath 'build\bin\Debug\Exporter.exe' 

$vertexPath = Join-Path -Path $currentPath -ChildPath '..\data\shaders\candles.vert.spv'
$fragmentPath = Join-Path -Path $currentPath -ChildPath '..\data\shaders\candles.frag.spv' 

$gltfPath = Join-Path -Path $currentPath -ChildPath '..\data\models\candles\scene.gltf'
$binaryPath = Join-Path -Path $currentPath -ChildPath '..\data\models\candles\scene.bin'

& $exePath $vertexPath $fragmentPath $gltfPath $binaryPath
# & $exePath $gltfPath $binaryPath

# Combine the arguments into a single string
# $arguments = $vertexPath $fragmentPath

# Run the executable with the arguments
# Start-Process -FilePath $exePath -ArgumentList $arguments -NoNewWindow -Wait
