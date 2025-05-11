@echo off
for %%f in (*.png) do (
    texconv.exe -f DXT5 -o . "%%f"
)
pause