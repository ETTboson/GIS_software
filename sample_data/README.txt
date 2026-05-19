sample_table.csv
- Type: CSV numeric table
- Shape: 4 rows x 4 cols
- Values: 1..16
- Expected basic statistics:
  - min = 1
  - max = 16
  - mean = 8.5

sample_raster.asc
- Type: simple raster text (ESRI ASCII Grid style)
- Shape: 5 rows x 5 cols
- Expected basic statistics:
  - min = 1
  - max = 6
  - mean = 3.2
- Suggested neighborhood test:
  - window size = 3
  - center cell original value = 5

sample_spatialite.sqlite
- Type: SpatiaLite / SQLite spatial database
- CRS: EPSG:4326
- Spatial tables:
  - poi_points: 5 point features, fields name/category/score
  - planning_blocks: 2 polygon features, fields name/block_type/priority
  - protected_areas: 2 polygon features, fields name/area_type/sensitivity
- Suggested map test:
  - Add Layer -> choose sample_spatialite.sqlite
  - Expected layers: poi_points, planning_blocks, protected_areas
- Suggested analysis test:
  - Open Data -> choose sample_spatialite.sqlite
  - Buffer test: run buffer on poi_points
  - Overlay test: run Intersect between planning_blocks and protected_areas

create_sample_spatialite.sql
- Type: sqlite3 script used to recreate sample_spatialite.sqlite
- Requires mod_spatialite from D:/OSGeo4W/bin/mod_spatialite
