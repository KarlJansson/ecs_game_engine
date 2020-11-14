mkdir tmp_texture_packs
mkdir tmp_models_packs

rmdir ..\content\ /S /Q

xcopy .\content ..\content /s /i

..\_tools_\win\fract_cooking.exe td ./textures o ./tmp_texture_packs/stock
..\_tools_\win\fract_cooking.exe md ./models o ./tmp_models_packs/stock

xcopy ".\tmp_texture_packs\stock_texpack" "..\content\stock_texpack*" /Y

xcopy ".\tmp_models_packs\stock_modelpack" "..\content\stock_modelpack*" /Y

xcopy ..\app_example\source\example_script.txt ..\content\example_script.txt* /Y

rmdir tmp_models_packs /S /Q
rmdir tmp_texture_packs /S /Q
