@REM Build the C library (lib\hydra.lib) and the samples (bin\*.hyx), as build.js does on any OS
@node "%~dp0..\..\build.js" c %*
