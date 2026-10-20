foreach ($module in 'environment', 'signals', 'assert', 'function-name', 'hosted-main', 'arguments') {
    . (Join-Path $PSScriptRoot ($module + '.ps1'))
}
