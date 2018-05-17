mkdir tmp_texture_packs
mkdir tmp_models_packs

rmdir ..\_build_release_\app_example\content\ /S /Q
rmdir ..\_build_release_\app_example\Build_Output\bin\Debug\content\ /S /Q
rmdir ..\_build_release_\app_example\Build_Output\bin\Release\content\ /S /Q
rmdir ..\_build_release_\app_example\Build_Output\bin\RelWithDebInfo\content\ /S /Q

xcopy .\content ..\_build_release_\app_example\content /s /i
xcopy .\content ..\_build_release_\app_example\Build_Output\bin\Debug\content /s /i
xcopy .\content ..\_build_release_\app_example\Build_Output\bin\Release\content /s /i
xcopy .\content ..\_build_release_\app_example\Build_Output\bin\RelWithDebInfo\content /s /i

..\_tools_\win\fract_cooking.exe td ./textures o ./tmp_texture_packs/stock
..\_tools_\win\fract_cooking.exe md ./models o ./tmp_models_packs/stock

xcopy ".\tmp_texture_packs\stock_texpack" "..\_build_release_\app_example\content\stock_texpack*" /Y
xcopy ".\tmp_texture_packs\stock_texpack" "..\_build_release_\app_example\Build_Output\bin\Debug\content\stock_texpack*" /Y
xcopy ".\tmp_texture_packs\stock_texpack" "..\_build_release_\app_example\Build_Output\bin\Release\content\stock_texpack*" /Y
xcopy ".\tmp_texture_packs\stock_texpack" "..\_build_release_\app_example\Build_Output\bin\RelWithDebInfo\content\stock_texpack*" /Y

xcopy ".\tmp_models_packs\stock_modelpack" "..\_build_release_\app_example\content\stock_modelpack*" /Y
xcopy ".\tmp_models_packs\stock_modelpack" "..\_build_release_\app_example\Build_Output\bin\Debug\content\stock_modelpack*" /Y
xcopy ".\tmp_models_packs\stock_modelpack" "..\_build_release_\app_example\Build_Output\bin\Release\content\stock_modelpack*" /Y
xcopy ".\tmp_models_packs\stock_modelpack" "..\_build_release_\app_example\Build_Output\bin\RelWithDebInfo\content\stock_modelpack*" /Y

xcopy ..\app_example\source\example_script.txt ..\_build_release_\app_example\content\example_script.txt* /Y
xcopy ..\app_example\source\example_script.txt ..\_build_release_\app_example\Build_Output\bin\Debug\content\example_script.txt* /Y
xcopy ..\app_example\source\example_script.txt ..\_build_release_\app_example\Build_Output\bin\Release\content\example_script.txt* /Y
xcopy ..\app_example\source\example_script.txt ..\_build_release_\app_example\Build_Output\bin\RelWithDebInfo\content\example_script.txt* /Y

rmdir tmp_models_packs /S /Q
rmdir tmp_texture_packs /S /Q
