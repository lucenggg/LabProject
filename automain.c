#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "open_interface.h"
#include "movement.h"
#include "adc.h"
#include "servo.h"
#include "uart.h"

//  CONSTANTS
#define MAX_OBJECTS 20
#define MAX_CLUSTERS 10

#define SMALL_WIDTH 10
#define CLUSTER_GAP_DEG 20
#define MAX_DISTANCE 100

#define FORWARD_SPEED 100

//  STRUCTS
typedef struct {
    int start_angle;
    int end_angle;
    int mid_angle;
    float distance;
    float width;
    bool is_thin;
} object_t;

typedef struct {
    int count;
    int mid_angle;
} cluster_t;

//  GLOBALS
object_t objects[MAX_OBJECTS];
cluster_t clusters[MAX_CLUSTERS];

int object_count = 0;
int cluster_count = 0;

oi_t *sensor_data;

//  FUNCTION PROTOTYPES
void perform_scan(void);
void detect_objects(void);
void classify_objects(void);
void build_clusters(void);

int find_valid_cluster(void);
bool large_pillar_nearby(int angle);

void navigate_to_cluster(int index);
void roam(void);

bool boundary_detected(void);

// MAIN
int main(void)
{
    sensor_data = oi_alloc();
    oi_init(sensor_data);

    adc_init();
    servo_init();
    uart_init();

    while (1)
    {
        perform_scan();
        detect_objects();
        classify_objects();
        build_clusters();

        int target = find_valid_cluster();

        if (target != -1)
        {
            navigate_to_cluster(target);
        }
        else
        {
            roam();
        }
    }

    oi_free(sensor_data);
    return 0;
}

// SCAN
void perform_scan(void)
{
    object_count = 0;

    for (int angle = 0; angle <= 180; angle += 2)
    {
        servo_move(angle);

        int ir = adc_read();

        // Simple detection threshold
        if (ir < 2000)
        {
            if (object_count < MAX_OBJECTS)
            {
                objects[object_count].start_angle = angle;
                objects[object_count].distance = ir;
                objects[object_count].end_angle = angle;
                object_count++;
            }
        }
    }
}

// DETECT OBJECT WIDTH
void detect_objects(void)
{
    for (int i = 0; i < object_count; i++)
    {
        int angular_width = objects[i].end_angle - objects[i].start_angle;

        objects[i].mid_angle = (objects[i].start_angle + objects[i].end_angle) / 2;

        // crude width estimation
        objects[i].width = angular_width * objects[i].distance * 0.01745;
    }
}

// CLASSIFY
void classify_objects(void)
{
    for (int i = 0; i < object_count; i++)
    {
        if (objects[i].width < SMALL_WIDTH)
            objects[i].is_thin = true;
        else
            objects[i].is_thin = false;
    }
}

// BUILD CLUSTERS
void build_clusters(void)
{
    cluster_count = 0;

    for (int i = 0; i < object_count; i++)
    {
        if (!objects[i].is_thin || objects[i].distance > MAX_DISTANCE)
            continue;

        bool added = false;

        for (int j = 0; j < cluster_count; j++)
        {
            if (abs(objects[i].mid_angle - clusters[j].mid_angle) < CLUSTER_GAP_DEG)
            {
                clusters[j].count++;
                clusters[j].mid_angle =
                    (clusters[j].mid_angle + objects[i].mid_angle) / 2;

                added = true;
                break;
            }
        }

        if (!added && cluster_count < MAX_CLUSTERS)
        {
            clusters[cluster_count].count = 1;
            clusters[cluster_count].mid_angle = objects[i].mid_angle;
            cluster_count++;
        }
    }
}

// TARGET SELECTION
int find_valid_cluster(void)
{
    int best_index = -1;
    int best_size = 0;

    for (int i = 0; i < cluster_count; i++)
    {
        if (large_pillar_nearby(clusters[i].mid_angle))
            continue;

        if (clusters[i].count > best_size)
        {
            best_size = clusters[i].count;
            best_index = i;
        }
    }

    return best_index;
}

// LARGE PILLAR CHECK
bool large_pillar_nearby(int target_angle)
{
    for (int i = 0; i < object_count; i++)
    {
        if (!objects[i].is_thin)
        {
            int diff = abs(objects[i].mid_angle - target_angle);

            if (diff < 20)
                return true;
        }
    }
    return false;
}

// NAVIGATION
void navigate_to_cluster(int index)
{
    int target_angle = clusters[index].mid_angle;

    turn_left(sensor_data, target_angle);

    while (1)
    {
        oi_update(sensor_data);

        if (boundary_detected())
        {
            oi_setWheels(0, 0);
            move_backward(sensor_data, 100);
            turn_left(sensor_data, 90);
            return;
        }

        if (sensor_data->distance > 150)
        {
            oi_setWheels(0, 0);
            return;
        }

        oi_setWheels(FORWARD_SPEED, FORWARD_SPEED);
    }
}

// ROAM
void roam(void)
{
    move_forward(sensor_data, 150);

    int angle = rand() % 90 + 45;
    turn_left(sensor_data, angle);
}

// BOUNDARY
bool boundary_detected(void)
{
    oi_update(sensor_data);

    return (
        sensor_data->cliffFrontLeftSignal < 2000 ||
        sensor_data->cliffFrontRightSignal < 2000 ||
        sensor_data->cliffLeftSignal < 2000 ||
        sensor_data->cliffRightSignal < 2000
    );
}
