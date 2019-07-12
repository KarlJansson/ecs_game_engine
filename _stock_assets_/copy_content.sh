bp=../_build_release_/app_ecs_game_engine/Build_Output/bin
bp_dbg=../_build_debug_/app_ecs_game_engine/Build_Output/bin

cp -r ./content/* $bp"/content/"
cp -r ./content/* $bp_dbg"/content/"

cp ../app_ecs_game_engine/source/example_script.txt $bp"/content/"
cp ../app_ecs_game_engine/source/example_script.txt $bp_dbg"/content/"
