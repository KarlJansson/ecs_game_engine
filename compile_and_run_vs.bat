.\_tools_\win\cmakemaker.exe

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

if not exist .\_build_release_\ (
  mkdir .\_build_release_\ 
)

cd .\_stock_assets_\
call compile_assets.bat
cd ..

cd .\_build_release_\ 
cmake -G "Visual Studio 16 2019" -A x64 .. 
devenv cmakemaker_solution.sln /Build Release

cd .\app_example\Build_Output\bin\Release 
ecs_game_engine.exe
cd ..\..\..\..\..