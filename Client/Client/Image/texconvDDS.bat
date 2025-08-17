@echo off
for %%f in (*.png) do (
    texconv.exe -f DXT5 -alpha -o . "%%f"
)
pause