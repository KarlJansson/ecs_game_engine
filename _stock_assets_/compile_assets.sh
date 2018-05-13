bp=../_build_release_/app_example/Build_Output/bin
bp_dbg=../_build_debug_/app_example/Build_Output/bin


if [ -d "$bp/content" ]
then
  echo ""
else
  mkdir $bp/content
fi

if [ -d "$bp_dbg/content" ]
then
  echo ""
else
  mkdir $bp_dbg/content
fi

mkdir tmp_texture_packs
mkdir tmp_models_packs

for i in textures
do
  fract_cooking td ./$i o ./tmp_texture_packs/$i
done

for i in models
do
  fract_cooking md ./$i o ./tmp_models_packs/$i
done


for i in textures
do
  cp "./tmp_texture_packs/"$i"_texpack" $bp"/content/stock_texpack"
  cp "./tmp_texture_packs/"$i"_texpack" $bp_dbg"/content/stock_texpack"
done

for i in models
do
  cp "./tmp_models_packs/"$i"_modelpack" $bp"/content/stock_modelpack"
  cp "./tmp_models_packs/"$i"_modelpack" $bp_dbg"/content/stock_modelpack"
done


cp ./content/* $bp"/content/"
cp ./content/* $bp_dbg"/content/"

cp ../app_example/source/example_script.txt $bp"/content/"
cp ../app_example/source/example_script.txt $bp_dbg"/content/"


rm -rf tmp_models_packs
rm -rf tmp_texture_packs
