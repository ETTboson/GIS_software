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
