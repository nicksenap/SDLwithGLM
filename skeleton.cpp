//--------------------LAB3-PART-1--------------------//

#include <iostream>
#include "glm/glm.hpp"
#include "SDL.h"
#include "SDLauxiliary.h"
#include "TestModel.h"

using namespace std;
using glm::vec3;
using glm::vec2;
using glm::ivec2;
using glm::mat3;

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

const int SCREEN_WIDTH = 200;
const int SCREEN_HEIGHT = 200;
SDL_Surface* screen;

int t;
int ROWS;


vector<Triangle> triangles;

vec3 currentColor;
vec3 cameraPos(0, 0,-3.001);
mat3 R;             //Rotation matrix;

float yaw = 0;      //Y-axis
float cameraAngle;  //X-axis
float tit;          //Z-axis

float focalLength = SCREEN_HEIGHT;
float depthBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];


// ----------------------------------------------------------------------------
// FUNCTIONS

void Update();
void Draw();
void VertexShader(const vec3& v, ivec2& p);
void Interpolate(ivec2 a, ivec2 b, vector<ivec2>& result);
void DrawLineSDL(SDL_Surface* surface, ivec2 a, ivec2 b, vec3 color);
void DrawPolygonEdges(const vector<vec3>& vertices);
void Rotate(vec3& in);
void ComputePolygonRows(const vector<ivec2>& vertexPixels,vector<ivec2>& leftPixels,vector<ivec2>& rightPixels);

void DrawRows(const vector<ivec2>& leftPixels,const vector<ivec2>& rightPixels);
void DrawPolygon(const vector<vec3>& vertices);
void DrawPolygonRows(vector<ivec2>leftPixels, vector<ivec2>rightPixels);

int main(int argc, char* argv[])
{
    
//    vector<ivec2> vertexPixels(3);
//    vertexPixels[0] = ivec2(10, 5);
//    vertexPixels[1] = ivec2( 5,10);
//    vertexPixels[2] = ivec2(15,15);
//    vector<ivec2> leftPixels;
//    vector<ivec2> rightPixels;
//    ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
//    for( int row=0; row<leftPixels.size(); ++row ){
//        cout << "Start: ("
//        << leftPixels[row].x << ","
//        << leftPixels[row].y << "). "
//        << "End: ("
//        << rightPixels[row].x << ","
//        << rightPixels[row].y << "). " << endl;
//    }
    
    
    LoadTestModel(triangles);
    screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);
    t = SDL_GetTicks();      // Set start value for timer.
    
    while (NoQuitMessageSDL())
    {
        Update();
        Draw();
    }

    
    return 0;
    
}

void Update()
{
    Uint8* keystate = SDL_GetKeyState(0);
    
    if (keystate[SDLK_UP])
        yaw+=0.01;
    
    if (keystate[SDLK_DOWN])
        yaw -= 0.01;
    
    if (keystate[SDLK_RIGHT])
        cameraAngle += 0.01;
    
    if (keystate[SDLK_LEFT])
        cameraAngle -= 0.01;
    
    if (keystate[SDLK_RSHIFT])
        tit += 0.01;
    
    if (keystate[SDLK_RCTRL])
        tit -= 0.01;
    
    if (keystate[SDLK_w])
        cameraPos.y -= 0.01;
    
    if (keystate[SDLK_s])
        cameraPos.y += 0.01;
    
    if (keystate[SDLK_d])
        cameraPos.x += 0.01;
    
    if (keystate[SDLK_a])
        cameraPos.x -= 0.01;
    
    if (keystate[SDLK_e])
        cameraPos.z += 0.01;
    
    if (keystate[SDLK_q])
        cameraPos.z -= 0.01;
    
    int dx;
    int dy;
    SDL_GetRelativeMouseState(&dx, &dy);
    
    SDL_PumpEvents();
    
    if (SDL_GetMouseState(NULL, NULL)&SDL_BUTTON(1))
    {
        cameraAngle -= (float)dx / 100;
        yaw += (float)dy / 100;
    }
        
}


void Draw()
{
    SDL_FillRect(screen, 0, 0);
    if (SDL_MUSTLOCK(screen))
        SDL_LockSurface(screen);
    for (int i = 0; i<triangles.size(); ++i)
    {
        vector<vec3> vertices(3);
        vertices[0] = triangles[i].v0;
        vertices[1] = triangles[i].v1;
        vertices[2] = triangles[i].v2;
        
        Rotate(vertices[0]);
        Rotate(vertices[1]);
        Rotate(vertices[2]);
        
        currentColor = triangles[i].color;
        DrawPolygon(vertices);
        //DrawPolygonEdges(vertices);
        
    }
    if (SDL_MUSTLOCK(screen))
        SDL_UnlockSurface(screen);
        SDL_UpdateRect(screen, 0, 0, 0, 0);
}

void VertexShader(const vec3& v, ivec2& p)
{
    vec3 c = (v - cameraPos);
    p.x = focalLength * (c.x / c.z) + (SCREEN_WIDTH / 2);
    p.y = focalLength * (c.y / c.z) + (SCREEN_HEIGHT / 2);
}

void Interpolate(ivec2 a, ivec2 b, vector<ivec2>& result)
{
    int N = (int)result.size();
    vec2 step = vec2(b-a) / float(glm::max(N-1, 1));
    vec2 current(a);
    for (int i = 0; i<N; ++i)
    {
        result[i] = current;
        current += step;
    }
}

void ComputePolygonRows(const vector<ivec2>& vertexPixels,vector<ivec2>& leftPixels,vector<ivec2>& rightPixels)
{
    {
        // 1. Find max and min y-value of the polygon
        // and compute the number of rows it occupies.
        
        int max = numeric_limits<int>::min();
        int min = numeric_limits<int>::max();
        
        for (ivec2 p : vertexPixels)
        {
            min = SDL_min(p.y, min);
            max = SDL_max(p.y, max);
        }
        
        // 2. Resize leftPixels and rightPixels
        // so that they have an element for each row.
        
        leftPixels.resize(max - min + 1);
        rightPixels.resize(max - min + 1);
        
        // 3. Initialize the x-	coordinates in leftPixels
        // to some really large value and the x-coordinates
        // in rightPixels to some really small value.
        
        for (int i = 0; i < leftPixels.size(); i++)
        {
            leftPixels[i] = ivec2(SCREEN_WIDTH, min + i);
            rightPixels[i] = ivec2(0, min + i);
        }
        
        // 4. Loop through all edges of	the polygon and use
        // linear interpolation to find the x-coordinate for
        // each row it occupies. Update the corresponding
        // values in rightPixels and leftPixels.
        
        for (int i = 0; i<vertexPixels.size(); i++)
        {
            ivec2 p1 = vertexPixels[i];
            ivec2 p2 = vertexPixels[(i + 1) % vertexPixels.size()];
            
            int steps = abs(p1.y-p2.y)+1;
            vector<ivec2> edge = vector<ivec2>(steps);
            Interpolate(p1, p2, edge);
            
            for (ivec2 p : edge)
            {
                int i = p.y-min;
                if (p.x < leftPixels[i].x)
                    leftPixels[i].x = p.x;
                if (p.x > rightPixels[i].x)
                    rightPixels[i].x = p.x;
                
            }
        }

    }

}

void Rotate(vec3& in)
{
    
    // Rotate around y axis
    vec3 row1(cos(cameraAngle), 0, sin(cameraAngle));
    vec3 row2(0, 1, 0);
    vec3 row3(-sin(cameraAngle), 0, cos(cameraAngle));
    
    float x = glm::dot(row1, in);
    float y = glm::dot(row2, in);
    float z = glm::dot(row3, in);
    
    in.x = x;
    in.y = y;
    in.z = z;
    
    // Rotate around x axis
    row1 = vec3(1, 0, 0);
    row2 = vec3(0, cos(yaw), -sin(yaw));
    row3 = vec3(0, sin(yaw), cos(yaw));
    
    x = glm::dot(row1, in);
    y = glm::dot(row2, in);
    z = glm::dot(row3, in);
    
    in.x = x;
    in.y = y;
    in.z = z;
    
    // Rotate around z axis
    row1 = vec3(cos(tit), -sin(tit), 0);
    row2 = vec3(sin(tit), cos(tit),0);
    row3 = vec3(0, 0, 1);
    
    x = glm::dot(row1, in);
    y = glm::dot(row2, in);
    z = glm::dot(row3, in);
    
    in.x = x;
    in.y = y;
    in.z = z;
}

void DrawPolygonEdges( const vector<vec3>& vertices )
{
    int V = (int)vertices.size();
    //Transform each vertex from 3D world position to 2D image position:
    vector<ivec2> projectedVertices( V );
    for( int i=0; i<V; ++i )
    {
        VertexShader( vertices[i], projectedVertices[i] );
    }
    //Loop over all vertices and draw the edge from it to the next vertex:
    for( int i=0; i<V; ++i )
    {
        int j = (i + 1) % V;        //The next vertex
        vec3 color( 1, 1, 1 );
        DrawLineSDL( screen, projectedVertices[i], projectedVertices[j], color);
    }
}


void DrawPolygonRows(vector<ivec2>leftPixels, vector<ivec2>rightPixels)
{
    for (int i = 0; i< leftPixels.size(); i++)
        DrawLineSDL(screen, leftPixels[i], rightPixels[i], currentColor);
}



void DrawLineSDL(SDL_Surface* surface, ivec2 a, ivec2 b, vec3 color)
{
    ivec2 delta = glm::abs(a-b);
    int pixels = glm::max(delta.x, delta.y) + 1;
    
    vector<ivec2> line(pixels);
    Interpolate(a, b, line);
    
    for (int i = 0; i < line.size(); i++) {
        PutPixelSDL(screen, line[i].x, line[i].y, color);
    }
}

void DrawPolygon( const vector<vec3>& vertices )
{
    int V = (int)vertices.size();
    vector<ivec2> vertexPixels( V );
    
    for( int i=0; i<V; ++i )
    {
        VertexShader( vertices[i], vertexPixels[i] );
    }
    
    vector<ivec2> leftPixels,rightPixels;
    ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
    DrawPolygonRows( leftPixels, rightPixels );
}

////-------------------------------LAB2-PART-II-----------------------------------------//
//
//
//#include <iostream>
//#include "glm/glm.hpp"
//#include "SDL.h"
//#include "SDLauxiliary.h"
//#include "TestModel.h"
//
//using namespace std;
//using glm::vec3;
//using glm::mat3;
//
//
////---------------------------------------------------------------------------
////STRUCTURES
//struct Intersection
//{
//    vec3 position;
//    float distance;
//    int triangleIndex;
//};
//
//
//// ----------------------------------------------------------------------------
//// GLOBAL VARIABLES
//
//const int SCREEN_WIDTH = 50;
//const int SCREEN_HEIGHT = 50;
//
//SDL_Surface* screen;
//int t;
//vector<Triangle> triangles;
//
////Camera
//float focalLength;
//vec3 campos;
//vec3 defCampos;
//
//float cameraAngle = 0;
//float yaw = 0;
//
////Lights
//vec3 lightPos(0, -0.5, -0.7);
//vec3 lightColor = 14.f * vec3(1, 1, 1);
//vec3 indirectLight = 0.5f * vec3(1, 1, 1);
//
//int bounces = 1;
//float r = 2;
//
//
//// ----------------------------------------------------------------------------
//// FUNCTIONS
//
//bool ClosestIntersection(vec3 start,vec3 dir,const vector<Triangle>& triangles,Intersection& closestIntersection);
//void Update();
//void Draw();
//
//void Rotate(vec3& in);
//
//vec3 DirectLight(const Intersection& i);
//
//vec3 cramersRule(vec3 col1,vec3 col2,vec3 col3, vec3 b);
//double det(vec3 col1, vec3 col2, vec3 col3);
//
//
//int main(int argc, char* argv[])
//{
//    screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);
//    LoadTestModel(triangles);
//    
//    campos = vec3(0, 0, -3);
//    defCampos = vec3(0, 0, -3);
//    
//    focalLength = SCREEN_WIDTH;
//    
//    t = SDL_GetTicks();	//Set start value for timer.
//    
//    while (NoQuitMessageSDL())
//    {
//        Update();
//        Draw();
//    }
//    
//    return 0;
//}
//
//void Update()
//{
//    Uint8* keystate = SDL_GetKeyState(0);
//    if (keystate[SDLK_UP])
//    {
//        yaw -= 0.1;
//        campos = defCampos;
//        Rotate(campos);
//    }
//    if (keystate[SDLK_DOWN])
//    {
//        yaw += 0.1;
//        campos = defCampos;
//        Rotate(campos);
//    }
//    if (keystate[SDLK_LEFT])
//    {
//        cameraAngle += 0.1;
//        campos = defCampos;
//        Rotate(campos);
//    }
//    if (keystate[SDLK_RIGHT])
//    {
//        cameraAngle -= 0.1;
//        campos = defCampos;
//        Rotate(campos);
//    }
//    if (keystate[SDLK_SPACE]) //Reset everything.
//    {
//        campos = vec3(0, 0, -3);
//        cameraAngle = 0;
//        yaw = 0;
//        lightPos = vec3(0, -0.5, -0.7);
//        lightColor = 14.f * vec3(1, 1, 1);
//    }
//    
//    if (keystate[SDLK_w])
//    {
//        lightPos.z += 0.1;
//    }
//    if (keystate[SDLK_s])
//    {
//        lightPos.z -= 0.1;
//    }
//    if (keystate[SDLK_d])
//    {
//        lightPos.x += 0.1;
//    }
//    if (keystate[SDLK_a])
//    {
//        lightPos.x-= 0.1;
//    }
//    if (keystate[SDLK_x])
//    {
//        lightColor = lightColor * 0.8f;
//    }
//    if (keystate[SDLK_z])
//    {
//        lightColor = lightColor * 1.2f;
//    }
//    if (keystate[SDLK_e]) //Zoom in
//    {
//        float l = sqrt(glm::dot(defCampos, defCampos));
//        if (l >= 1.5) {
//            float nl = l - 0.3;
//            defCampos = defCampos*(nl / l);
//            campos = defCampos;
//            Rotate(campos);
//        }
//    }
//    if (keystate[SDLK_q]) //Zoom out
//    {
//        float l = sqrt(glm::dot(defCampos, defCampos));
//        if (l > 0) {
//            float nl = l + 0.3;
//            defCampos = defCampos*(nl / l);
//            campos = defCampos;
//            Rotate(campos);
//        }
//    }
//    
//}
//
//void Draw()
//{
//    if (SDL_MUSTLOCK(screen))
//        SDL_LockSurface(screen);
//    
//    Intersection closest;
//    for (int y = 0; y<SCREEN_HEIGHT; ++y)
//    {
//        for (int x = 0; x<SCREEN_WIDTH; ++x)
//        {
//            vec3 dir(x - (SCREEN_WIDTH / 2), y - (SCREEN_HEIGHT / 2), focalLength);
//            dir = glm::normalize(dir);
//            
//            Rotate(dir); //Make the camera.
//            
//            bool closestIntersection = ClosestIntersection(campos, dir, triangles, closest);
//            
//            if (closestIntersection)
//            {
//                vec3 Dlight = DirectLight(closest);
//                vec3 color = triangles[closest.triangleIndex].color * ( Dlight + indirectLight );
//                PutPixelSDL(screen, x, y, color);
//            }
//            
//            else {
//                PutPixelSDL(screen, x, y, vec3(0,0,0));
//            }
//        }
//        SDL_UpdateRect(screen, 0, 0, 0, 0);
//    }
//    
//    if (SDL_MUSTLOCK(screen))
//        SDL_UnlockSurface(screen);
//    
//    SDL_UpdateRect(screen, 0, 0, 0, 0);
//    }
//
//
//bool ClosestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closestIntersection)
//{
//    bool first = true;
//    int index = 0;
//    float distance = 0.0;
//    vec3 pos;
//    
//    for (int i = 0; i < triangles.size(); i++)
//    {
//        Triangle triangle = triangles[i];
//        vec3 v0 = triangle.v0;
//        vec3 v1 = triangle.v1;
//        vec3 v2 = triangle.v2;
//        vec3 e1 = v1 - v0;
//        vec3 e2 = v2 - v0;
//        vec3 b = start - v0;
//        vec3 x = cramersRule(-dir,e1,e2,b);
//        
//        float t = x.x;
//        float u = x.y;
//        float v = x.z;
//        
//        if ((v + u) <= 1 && v >= 0 && u >= 0 && t>0) {
//            if (first || t < distance) {
//                index = i;
//                distance = t;
//                first = false;
//            }
//        }
//    }
//    
//    if (first == false){
//        closestIntersection.triangleIndex = index;
//        closestIntersection.distance = distance;
//        closestIntersection.position = start + distance * dir;
//        return true;
//    }
//    else {
//        return false;
//    }
//}
//
//
//void Rotate( vec3& in )
//{
//    //Matrix for rotate around y axis.
//    vec3 row1(cos(cameraAngle), 0, sin(cameraAngle));
//    vec3 row2(0, 1, 0);
//    vec3 row3(-sin(cameraAngle), 0, cos(cameraAngle));
//    
//    float x = glm::dot(row1, in);
//    float y = glm::dot(row2, in);
//    float z = glm::dot(row3, in);
//    
//    in.x = x;
//    in.y = y;
//    in.z = z;
//    
//    //Matrix for rotate around x axis.
//    row1 = vec3(1, 0, 0);
//    row2 = vec3(0, cos(yaw), -sin(yaw));
//    row3 = vec3(0, sin(yaw), cos(yaw));
//    
//    x = glm::dot(row1, in);
//    y = glm::dot(row2, in);
//    z = glm::dot(row3, in);
//    
//    in.x = x;
//    in.y = y;
//    in.z = z;
//}
//
//vec3 DirectLight( const Intersection& i )
//{
//    
//    vec3 n = glm::normalize(triangles[i.triangleIndex].normal);
//    vec3 dir = lightPos - i.position;
//    vec3 r = glm::normalize(dir);
//    
//    //A = 4πr^2
//    
//    float radius = glm::length(dir);
//    float area = 4 * 3.1415926 * radius * radius;
//    
//    //D = B max(r.n , 0) =( P max (r.n , 0)) / 4πr^2
//    //Then we let D do staffs to colors -> (lightColor * max)/area;
//    
//    float max = glm::max(glm::dot(r, n), 0.f);
//    vec3 D = lightColor * max/area;
//    
//    Intersection intersection;
//    int startIndex = i.triangleIndex;
//    ClosestIntersection(lightPos, i.position-lightPos, triangles, intersection);
//    
//    if(intersection.triangleIndex != startIndex && intersection.distance != 0){
//        
//        return vec3(0,0,0);
//    }
//    
//    return D;
//    
//}
//
//
//
////Pure Algebra things.
//
////Cramersregel
//vec3 cramersRule(vec3 col1, vec3 col2, vec3 col3, vec3 b)
//{
//    float A0 = det(col1, col2, col3);
//    float Ax = det(b, col2, col3);
//    float Ay = det(col1, b, col3);
//    float Az = det(col1, col2, b);
//    
//    return vec3(Ax/A0,Ay/A0,Az/A0);
//}
//
////Metode to get Determinant.
//double det(vec3 col1, vec3 col2, vec3 col3) 
//{
//    double plus = (col1[0] * col2[1] * col3[2]) + (col1[1] * col2[2] * col3[0]) + (col1[2] * col2[0] * col3[1]);
//    double minus = (col1[2] * col2[1] * col3[0]) + (col1[1] * col2[0] * col3[2]) + (col1[0] * col2[2] * col3[1]);
//    return plus - minus;
//}


////-------------------------------LAB2-PART-I-----------------------------------------//
//
//#include <iostream>
//#include "glm/glm.hpp"
//#include "SDL.h"
//#include "SDLauxiliary.h"
//#include "TestModel.h"
//
//using namespace std;
//using glm::vec3;
//using glm::mat3;
//
//
////---------------------------------------------------------------------------
//// STRUCTURES
//
//struct Intersection
//{
//    vec3 position;
//    float distance;
//    int triangleIndex;
//};
//
//
//// ----------------------------------------------------------------------------
//// GLOBAL VARIABLES
//
//const int SCREEN_WIDTH = 200;
//const int SCREEN_HEIGHT = 200;
//
//SDL_Surface* screen;
//vector<Triangle> triangles;
//
//int t;
//
//float focalLength;
//float cameraAngle = 0;
//float yaw = 0;
//
//vec3 campos;
//
//// ----------------------------------------------------------------------------
//// FUNCTIONS
//
//void Update();
//void Draw();
//bool ClosestIntersection(vec3 start,vec3 d,const vector<Triangle>& triangles,Intersection& closestIntersection);
//void Rotate(vec3& in);       // rev- option to mirror rotation
//
//int main(int argc, char* argv[])
//{
//    screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);
//    LoadTestModel(triangles);
//    
//    campos = vec3(0, 0, -3);
//    focalLength = SCREEN_WIDTH;
//    
//    t = SDL_GetTicks();         //Set start value for timer.
//    
//    while (NoQuitMessageSDL())
//    {
//        Update();
//        Draw();
//    }
//    
//    return 0;
//}
//
//void Update()
//{
//    // Compute frame time:
//    
//    int t2 = SDL_GetTicks();
//    float dt = float(t2 - t);
//    campos = vec3(0, 0, -3);
//    
//    t = t2;
//    if (dt < 20) {
//        SDL_Delay(20 - dt);
//    }
//    
//    //cout << "Render time: " << dt << " ms." << endl;
//    Uint8* keystate = SDL_GetKeyState(0);
//    
//    if (keystate[SDLK_UP])
//    {
//        yaw -= 0.05;
//        Rotate(campos);
//    }
//    if (keystate[SDLK_DOWN])
//    {
//        yaw += 0.05;
//        Rotate(campos);
//    }
//    if (keystate[SDLK_LEFT])
//    {
//        cameraAngle += 0.05;
//        Rotate(campos);
//    }
//    if (keystate[SDLK_RIGHT])
//    {
//        cameraAngle -= 0.05;
//        Rotate(campos);
//    }
//    if (keystate[SDLK_SPACE]) // reset
//    {
//        cameraAngle = 0;
//        yaw = 0;
//    }
//    
//}
//
//void Draw()
//{
//    if (SDL_MUSTLOCK(screen))
//        SDL_LockSurface(screen);
//    
//    Intersection closest;
//    for (int y = 0; y<SCREEN_HEIGHT; ++y)
//    {
//        for (int x = 0; x<SCREEN_WIDTH; ++x)
//        {
//            vec3 d(x - (SCREEN_WIDTH / 2), y - (SCREEN_HEIGHT / 2), focalLength); //＝＝SCREEN_WIDTH
//            d = glm::normalize(d);
//            Rotate(d);
//            
//            bool closestIntersection = ClosestIntersection(campos, d, triangles, closest);
//            
//            if (closestIntersection)
//            {
//                PutPixelSDL(screen,x,y,triangles[closest.triangleIndex].color);
//            }
//            
//            else {
//                PutPixelSDL(screen, x, y, vec3(0,0,0));
//            }
//        }
//        SDL_UpdateRect(screen, 0, 0, 0, 0);
//    }
//    
//    if (SDL_MUSTLOCK(screen))
//        SDL_UnlockSurface(screen);
//    
//    SDL_UpdateRect(screen, 0, 0, 0, 0);
//}
//
//
//bool ClosestIntersection(vec3 start, vec3 d, const vector<Triangle>& triangles, Intersection& closestIntersection)
//{
//    bool first = true;
//    int index = 0;
//    float distance = 0.0;
//    
//    for (int i = 0; i < triangles.size(); i++)
//    {
//        Triangle triangle = triangles[i];
//        
//        vec3 v0 = triangle.v0;
//        vec3 v1 = triangle.v1;
//        vec3 v2 = triangle.v2;
//        
//        vec3 e1 = v1 - v0;
//        vec3 e2 = v2 - v0;
//        
//        vec3 b = start - v0;
//        mat3 A(- d, e1, e2);
//        vec3 vec = glm::inverse(A) * b;
//        
//        float r = abs(vec.x);
//        float u = vec.y;
//        float v = vec.z;
//        
//        if ((v + u) <= 1 && v >= 0 && u >= 0) {
//            if (first || r < distance) {
//                index = i;
//                distance = r;
//                first = false;
//            }
//        }
//    }
//    
//    
//    if (first == false){
//        
//        closestIntersection.triangleIndex = index;
//        return true;
//    }
//    else {
//        return false;
//    }
//}
//
//void Rotate(vec3& in) {
//    
//    // Rotate around y axis
//    vec3 row1(cos(cameraAngle), 0, sin(cameraAngle));
//    vec3 row2(0, 1, 0);
//    vec3 row3(-sin(cameraAngle), 0, cos(cameraAngle));
//    
//    float x = glm::dot(row1, in);
//    float y = glm::dot(row2, in);
//    float z = glm::dot(row3, in);
//    
//    in.x = x;
//    in.y = y;
//    in.z = z;
//    
//    // Rotate around x axis
//    row1 = vec3(1, 0, 0);
//    row2 = vec3(0, cos(yaw), -sin(yaw));
//    row3 = vec3(0, sin(yaw), cos(yaw));
//    
//    x = glm::dot(row1, in);
//    y = glm::dot(row2, in);
//    z = glm::dot(row3, in);
//    
//    in.x = x;
//    in.y = y;
//    in.z = z;
//    
//}






////-----------------------------------------LAB1-Part-II--------------------------------------------//
//
//
//// Introduction lab that covers:
//// * C++
//// * SDL
//// * 2D graphics
//// * Plotting pixels
//// * Video memory
//// * Color representation
//// * Linear interpolation
//// * glm::vec3 and std::vector
//
//
//#include "SDL.h"
//#include <iostream>
//#include <string>
//#include "glm/glm.hpp"
//#include <vector>
//#include "SDLauxiliary.h"
//#include <unistd.h>
//#include <term.h>
//
//using namespace std;
//using glm::vec3;
//
//// --------------------------------------------------------
//// GLOBAL VARIABLES
//
//const int SCREEN_WIDTH = 640;
//const int SCREEN_HEIGHT = 480;
//SDL_Surface* screen;
//vector<vec3> stars;
//int  t;
//float v = 0.0009;
//int writefps;
//
//// --------------------------------------------------------
//// FUNCTION DECLARATIONS
//
//void Draw();
//void Update();
//void Interpolate(vec3 a, vec3 b, vector<vec3>& result);
//
//
//// --------------------------------------------------------
//// FUNCTION DEFINITIONS
//
//int main(int argc, char* argv[])
//{
////    cout << "number of stars" << endl;
////    string input = "";
////    getline(cin, input);
////    int nstars = atoi(input.c_str());
//    
//    screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);
//    //int nstars = 10000;
//    
//    cout << nstars << " stars are flying!!!" <<endl;
//    
//    vector<vec3> stars2(nstars);
//    stars = stars2; //Why?
//    
//    for (int i = 0; i < nstars; i++) {
//        stars[i].x =    float(rand()) / float(RAND_MAX) * 2 - 1;    //Random between -1 to 1
//        stars[i].y =    float(rand()) / float(RAND_MAX) * 2 - 1;    //Random between -1 to 1
//        stars[i].z =    float(rand()) / float(RAND_MAX);            //Random between  0 to 1
//    }
//    
//    t = SDL_GetTicks();
//    
//    while (NoQuitMessageSDL())
//    {
//        Update();
//        Draw();
//    }
//    
//    SDL_SaveBMP(screen, "screenshot2.bmp");
//    
//    return 0;
//}
//
//void Draw()
//{
//    
//    int f = SCREEN_HEIGHT / 2; //Focus length.
//    
//    SDL_FillRect(screen, 0, 0);
//    
//    if (SDL_MUSTLOCK(screen))
//        SDL_LockSurface(screen);
//    
//    for (size_t s = 0; s<stars.size(); s++)
//    {
//        //Add code for projecting and drawing each star.
//        
//        //Projection
//        float x = stars[s].x;
//        float y = stars[s].y;
//        float z = stars[s].z;
//        
//        int u = f*(x / z) + (SCREEN_WIDTH / 2);
//        int v = f*(y / z) + (SCREEN_HEIGHT / 2);
//    
//        //Colors should be different between the stars depends on the distans.
//        vec3 color = 0.1f * vec3(1, 1, 1) / (stars[s].z*stars[s].z);
//        
//        //vec3 color = vec3(1, 1, 1);
//        PutPixelSDL(screen, u, v, color);
//    }
//    if (SDL_MUSTLOCK(screen))
//        
//        SDL_UnlockSurface(screen);
//        SDL_UpdateRect(screen, 0, 0, 0, 0);
//}
//
//void Update() {
//    
//    int t2 = SDL_GetTicks();
//    float dt = float(t2 - t);
//    t = t2;
//    
//    if (dt < 16) {
//        SDL_Delay(16 - dt);
//    }
//    
//    //if ((writefps = (++writefps) % 10) == 0) {
//    //   system("clear");
//    // if (dt < 16) {
//    //   cout << "FPS " << 1000 / 16 << endl;
//    //}
//    //  else {
//    //  cout << "FPS " << 1000 / dt << endl;
//    //}
//    //}
//    
//    int n = (int)stars.size();
//    for (int i = 0; i < n; i++) {
//        
//        stars[i].z = stars[i].z - dt*v;
//        
//        if (stars[i].z <= 0)
//            stars[i].z += 1;
//        
//        if (stars[i].z > 1)
//            stars[i].z -= 1;
//    }
//}
//
//void Interpolate(vec3 a, vec3 b, vector<vec3>& result) {
//    int steps = (int)result.size() - 1;
//    
//    if (steps <= 0)
//        return;
//    
//    //Define this start and the ending point.
//    float startx = a.x;
//    float starty = a.y;
//    float startz = a.z;
//    float endx = b.x;
//    float endy = b.y;
//    float endz = b.z;
//    
//    //Calculate how much each step should take.
//    float stepx = (endx - startx) / steps;
//    float stepy = (endy - starty) / steps;
//    float stepz = (endz - startz) / steps;
//    
//    //Give the value with a for loop.
//    for (int i = 0; i <= steps; ++i) {
//        result[i].x = startx + i*stepx;
//        result[i].y = starty + i*stepy;
//        result[i].z = startz + i*stepz;
//    }
//
//}


////-----------------------------------------LAB1-Part-I--------------------------------------------------//
//
//
//
// 
// 
// // Introduction lab that covers:
// // * C++
// // * SDL
// // * 2D graphics
// // * Plotting pixels
// // * Video memory
// // * Color representation
// // * Linear interpolation
// // * glm::vec3 and std::vector
// 
// #include "SDL.h"
// #include <iostream>
// #include "glm/glm.hpp"
// #include <vector>
// #include "SDLauxiliary.h"
// 
// using namespace std;
// using glm::vec3;
// 
// // --------------------------------------------------------
// // GLOBAL VARIABLES
// 
// const int SCREEN_WIDTH = 640;
// const int SCREEN_HEIGHT = 480;
// SDL_Surface* screen;
// 
// // --------------------------------------------------------
// // FUNCTION DECLARATIONS
// 
// void Draw();
// void Interpolate(vec3 a, vec3 b, vector<vec3>& result);
// 
// // --------------------------------------------------------
// // FUNCTION DEFINITIONS
// 
// int main(int argc, char* argv[])
// {
// 
// screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);
// while (NoQuitMessageSDL())
// {
// Draw();
// SDL_Delay(200);
// }
// SDL_SaveBMP(screen, "part1.bmp");
// 
// return 0;
// }
// 
// 
// 
// 
// 
// void Draw()
// {
//    vec3 topLeft(1, 0, 0);      // red
//    vec3 topRight(0, 0, 1);     // blue
//    vec3 bottomLeft(0, 1, 0);   // green
//    vec3 bottomRight(1, 1, 0);  // yellow
// 
//    vector<vec3> leftSide(SCREEN_HEIGHT);
//    vector<vec3> rightSide(SCREEN_HEIGHT);
// 
//    Interpolate(topLeft, bottomLeft, leftSide);
//    Interpolate(topRight, bottomRight, rightSide);
// 
// for (int y = 0; y<SCREEN_HEIGHT; ++y)
// {
// 
// vector<vec3> colors(SCREEN_WIDTH);
// 
// Interpolate(leftSide[y], rightSide[y], colors);
// 
// for (int x = 0; x<SCREEN_WIDTH; ++x)
// {
//    PutPixelSDL(screen, x, y, colors[x]);
//    }
// }
// 
// if (SDL_MUSTLOCK(screen))
//    SDL_UnlockSurface(screen);
// 
//    SDL_UpdateRect(screen, 0, 0, 0, 0);
// }
// 
// void Interpolate(vec3 a, vec3 b, vector<vec3>& result) {
//    int steps = (int)result.size() - 1;
// 
//    if (steps <= 0)
//    return;
// 
//    //Define this start and the ending point.
//    float startx = a.x;
//    float starty = a.y;
//    float startz = a.z;
//    float endx = b.x;
//    float endy = b.y;
//    float endz = b.z;
// 
// //Calculate how much each step should take.
//    float stepx = (endx - startx) / steps;
//    float stepy = (endy - starty) / steps;
//    float stepz = (endz - startz) / steps;
// 
// //Give the value with a for loop.
//    for (int i = 0; i <= steps; ++i) {
//    result[i].x = startx + i*stepx;
//    result[i].y = starty + i*stepy;
//    result[i].z = startz + i*stepz;
//    }
// }




