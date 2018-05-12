.\_tools\win\cmakemaker.exe

if not exist .\_build_release_\ (
  mkdir .\_build_release_\ 
)

cd .\_build_release_\ 
cmake -G "Visual Studio 15 2017 Win64" .. 
cd .. 

cd .\_stock_assets_\
call compile_assets.bat
cd ..