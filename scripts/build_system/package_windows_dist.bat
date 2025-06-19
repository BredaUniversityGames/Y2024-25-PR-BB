cd ../..
python scripts/python/package.py -c "scripts/build_system/package_config/windows_dist.json" --distribution
cd scripts/build_system
echo Windows distribution build packaged.