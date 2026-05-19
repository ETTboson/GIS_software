.load D:/OSGeo4W/bin/mod_spatialite
PRAGMA trusted_schema=1;
SELECT InitSpatialMetaData(1);

CREATE TABLE poi_points (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    category TEXT NOT NULL,
    score REAL NOT NULL
);
SELECT AddGeometryColumn('poi_points', 'geom', 4326, 'POINT', 'XY');

INSERT INTO poi_points (name, category, score, geom)
VALUES
    ('Alpha Center', 'service', 92.5, MakePoint(0.0, 0.0, 4326)),
    ('Beta School', 'education', 81.0, MakePoint(0.8, 0.6, 4326)),
    ('Gamma Clinic', 'health', 76.5, MakePoint(1.6, 0.9, 4326)),
    ('Delta Depot', 'logistics', 68.0, MakePoint(2.4, 0.4, 4326)),
    ('Echo Park Gate', 'recreation', 88.0, MakePoint(-0.4, 1.6, 4326));
SELECT CreateSpatialIndex('poi_points', 'geom');

CREATE TABLE planning_blocks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    block_type TEXT NOT NULL,
    priority INTEGER NOT NULL
);
SELECT AddGeometryColumn('planning_blocks', 'geom', 4326, 'POLYGON', 'XY');

INSERT INTO planning_blocks (name, block_type, priority, geom)
VALUES
    ('West Block', 'mixed_use', 1,
        GeomFromText('POLYGON((-0.5 -0.5, 1.5 -0.5, 1.5 1.5, -0.5 1.5, -0.5 -0.5))', 4326)),
    ('East Block', 'industrial', 2,
        GeomFromText('POLYGON((1.0 -0.2, 3.0 -0.2, 3.0 1.8, 1.0 1.8, 1.0 -0.2))', 4326));
SELECT CreateSpatialIndex('planning_blocks', 'geom');

CREATE TABLE protected_areas (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    area_type TEXT NOT NULL,
    sensitivity INTEGER NOT NULL
);
SELECT AddGeometryColumn('protected_areas', 'geom', 4326, 'POLYGON', 'XY');

INSERT INTO protected_areas (name, area_type, sensitivity, geom)
VALUES
    ('Wetland A', 'wetland', 5,
        GeomFromText('POLYGON((0.4 0.4, 2.2 0.4, 2.2 1.4, 0.4 1.4, 0.4 0.4))', 4326)),
    ('Park B', 'park', 3,
        GeomFromText('POLYGON((-0.8 1.0, 0.8 1.0, 0.8 2.2, -0.8 2.2, -0.8 1.0))', 4326));
SELECT CreateSpatialIndex('protected_areas', 'geom');

SELECT 'poi_points' AS layer_name, Count(*) AS feature_count FROM poi_points
UNION ALL
SELECT 'planning_blocks', Count(*) FROM planning_blocks
UNION ALL
SELECT 'protected_areas', Count(*) FROM protected_areas;
