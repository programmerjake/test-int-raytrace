#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#ifdef __APPLE__
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

double Timer()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

SDL_Surface * screen = NULL;

const int XRes = 640, YRes = 480;

void SetPixel(int X, int Y, Uint32 color) // screen must be locked
{
    if(X < 0 || X >= XRes || Y < 0 || Y >= YRes) return;
    Uint8 * addr = (Uint8 *)screen->pixels;
    addr += Y * screen->pitch + X * screen->format->BytesPerPixel;
    Uint32 * ppixel = (Uint32 *)addr;
    *ppixel = color;
}

const int xzsz = 512, ysz = 128;
const int xscale = xzsz * ysz;
const int yscale = xzsz;

typedef Uint32 voxel_t;

const voxel_t emptyVoxel = (voxel_t)0xFFFFFFFF;

voxel_t voxels[xzsz * ysz * xzsz];

inline int getVoxelDeltaAddr(int x, int y, int z)
{
    return x * xscale + y * yscale + z;
}

inline voxel_t * getVoxelAddr(int x, int y, int z)
{
    return &voxels[getVoxelDeltaAddr(x, y, z)];
}

inline bool IsVoxelInsideArray(int x, int y, int z)
{
    if(x < 0 || x >= xzsz || y < 0 || y >= ysz || z < 0 || z >= xzsz)
        return false;
    return true;
}

inline void setVoxel(int x, int y, int z, voxel_t v)
{
    if(IsVoxelInsideArray(x, y, z))
        *getVoxelAddr(x, y, z) = v;
}

void InitVoxels()
{
//#error finish
    for(int x=0;x<xzsz;x++)
    {
        for(int y=0;y<ysz;y++)
        {
            for(int z=0;z<xzsz;z++)
            {
                int px = x - xzsz / 2;
                int py = y - ysz / 2;
                int pz = z - xzsz / 2;
                if(px * px + py * py * 3 * 3 + pz * pz < 20 * 20)
                {
                    setVoxel(x, y, z, emptyVoxel);
                }
                else
                {
                    setVoxel(x, y, z, rand() & 0xFFFFFF);
                }
            }
        }
    }
}

inline bool voxelEmpty(voxel_t * pvoxel)
{
    if(*pvoxel == emptyVoxel)
        return true;
    return false;
}

inline bool voxelOpaque(voxel_t * pvoxel)
{
    if(*pvoxel == emptyVoxel)
        return false;
    return true;
}

#define fractional_shift 16
#define STRINGIFY(a) #a
const int fractional_size = 1 << fractional_shift;
const int floor_mask = ~(fractional_size - 1);

inline int fixedMul(int a, int b)
{
#if 1
    int retval = (int)((int64_t)a * b >> fractional_shift);
#else
    int retval;
    asm(
        ""
        : "=&r" (retval));
#endif
    return retval;
}

voxel_t * IntTrace(float fdirx, float fdiry, float fdirz, float fposx, float fposy, float fposz, float * dist)
{
    if(dist) *dist = -1;
    int dirx, diry, dirz;
    int invdirx, invdiry, invdirz;
    int posx, posy, posz;
    float finvdirx, finvdiry, finvdirz;
    const float eps = 1e-3;
    if(fabs(fdirx) < eps)
        fdirx = eps;
    finvdirx = 1 / fdirx;
    if(fabs(fdiry) < eps)
        fdiry = eps;
    finvdiry = 1 / fdiry;
    if(fabs(fdirz) < eps)
        fdirz = eps;
    finvdirz = 1 / fdirz;
    posx = (int)(fposx * fractional_size);
    posy = (int)(fposy * fractional_size);
    posz = (int)(fposz * fractional_size);
    if(!IsVoxelInsideArray(posx >> fractional_shift, posy >> fractional_shift, posz >> fractional_shift))
        return NULL;
    dirx = (int)(fdirx * fractional_size);
    diry = (int)(fdiry * fractional_size);
    dirz = (int)(fdirz * fractional_size);
    invdirx = (int)(finvdirx * fractional_size);
    invdiry = (int)(finvdiry * fractional_size);
    invdirz = (int)(finvdirz * fractional_size);
    voxel_t * pvoxel = getVoxelAddr(posx >> fractional_shift, posy >> fractional_shift, posz >> fractional_shift);
    int totalt = 0;
    int nextxxinc = ((dirx < 0) ? -1 : fractional_size);
    int nextyyinc = ((diry < 0) ? -1 : fractional_size);
    int nextzzinc = ((dirz < 0) ? -1 : fractional_size);
    int fixx = 0, fixy = 0, fixz = 0;
    if(dirx < 0) fixx = -1;
    if(diry < 0) fixy = -1;
    if(dirz < 0) fixz = -1;
    const int maxt = (int)(fractional_size * 2000.0 / sqrt(fdirx * fdirx + fdiry * fdiry + fdirz * fdirz));
    int nextxx, nextyy, nextzz, xt, yt, zt;
    int nextxy, nextxz, nextyx, nextyz, nextzx, nextzy;
    nextxx = (posx + nextxxinc) & floor_mask;
    nextyy = (posy + nextyyinc) & floor_mask;
    nextzz = (posz + nextzzinc) & floor_mask;
    nextxxinc = ((dirx < 0) ? -fractional_size : fractional_size);
    nextyyinc = ((diry < 0) ? -fractional_size : fractional_size);
    nextzzinc = ((dirz < 0) ? -fractional_size : fractional_size);
    xt = fixedMul(nextxx - posx, invdirx);
    yt = fixedMul(nextyy - posy, invdiry);
    zt = fixedMul(nextzz - posz, invdirz);
    nextxy = fixedMul(xt, diry) + posy;
    nextxz = fixedMul(xt, dirz) + posz;
    nextyx = fixedMul(yt, dirx) + posx;
    nextyz = fixedMul(yt, dirz) + posz;
    nextzx = fixedMul(zt, dirx) + posx;
    nextzy = fixedMul(zt, diry) + posy;
    int nextxyinc, nextxzinc, nextyxinc, nextyzinc, nextzxinc, nextzyinc;
    int xtinc = abs(invdirx), ytinc = abs(invdiry), ztinc = abs(invdirz);
    nextxyinc = fixedMul(xtinc, diry);
    nextxzinc = fixedMul(xtinc, dirz);
    nextyxinc = fixedMul(ytinc, dirx);
    nextyzinc = fixedMul(ytinc, dirz);
    nextzxinc = fixedMul(ztinc, dirx);
    nextzyinc = fixedMul(ztinc, diry);
    while(voxelEmpty(pvoxel))
    {
        int t;
        int ix, iy, iz;
        int newposx, newposy, newposz;
        if(xt < yt)
        {
            if(xt < zt)
            {
                t = xt;
                newposx = nextxx;
                newposy = nextxy;
                newposz = nextxz;
                ix = newposx >> fractional_shift;
                iy = newposy >> fractional_shift;
                iz = newposz >> fractional_shift;
                ix += fixx;
                yt -= t;
                zt -= t;
                nextxx += nextxxinc;
                nextxy += nextxyinc;
                nextxz += nextxzinc;
                xt = xtinc;
            }
            else
            {
                t = zt;
                newposx = nextzx;
                newposy = nextzy;
                newposz = nextzz;
                ix = newposx >> fractional_shift;
                iy = newposy >> fractional_shift;
                iz = newposz >> fractional_shift;
                iz += fixz;
                xt -= t;
                yt -= t;
                nextzx += nextzxinc;
                nextzy += nextzyinc;
                nextzz += nextzzinc;
                zt = ztinc;
            }
        }
        else
        {
            if(yt < zt)
            {
                t = yt;
                newposx = nextyx;
                newposy = nextyy;
                newposz = nextyz;
                ix = newposx >> fractional_shift;
                iy = newposy >> fractional_shift;
                iz = newposz >> fractional_shift;
                iy += fixy;
                xt -= t;
                zt -= t;
                nextyx += nextyxinc;
                nextyy += nextyyinc;
                nextyz += nextyzinc;
                yt = ytinc;
            }
            else
            {
                t = zt;
                newposx = nextzx;
                newposy = nextzy;
                newposz = nextzz;
                ix = newposx >> fractional_shift;
                iy = newposy >> fractional_shift;
                iz = newposz >> fractional_shift;
                iz += fixz;
                xt -= t;
                yt -= t;
                nextzx += nextzxinc;
                nextzy += nextzyinc;
                nextzz += nextzzinc;
                zt = ztinc;
            }
        }
        posx = newposx;
        posy = newposy;
        posz = newposz;
        if(!IsVoxelInsideArray(ix, iy, iz))
            return NULL;
        pvoxel = getVoxelAddr(ix, iy, iz);
        totalt += t;
        if(totalt > maxt) return NULL;
    }
    if(dist) *dist = (float)totalt / fractional_size;
    return pvoxel;
}

inline void RotateX(float * /*X*/, float * Y, float * Z, float angle)
{
    float s = sin(angle), c = cos(angle);
    float yv = *Y * c - *Z * s;
    float zv = *Z * c + *Y * s;
    *Y = yv;
    *Z = zv;
}

inline void RotateY(float * X, float * /*Y*/, float * Z, float angle)
{
    float s = sin(angle), c = cos(angle);
    float xv = *X * c - *Z * s;
    float zv = *Z * c + *X * s;
    *X = xv;
    *Z = zv;
}

inline void RotateZ(float * X, float * Y, float * /*Z*/, float angle)
{
    float s = sin(angle), c = cos(angle);
    float xv = *X * c - *Y * s;
    float yv = *Y * c + *X * s;
    *X = xv;
    *Y = yv;
}

float viewtheta = 0, viewphi = 0;

void getDir(float * dirx, float * diry, float * dirz, int X, int Y)
{
    *dirx = (float)X / XRes * 2 - 1;
    *diry = 1 - (float)Y / YRes * 2;
    *dirz = 1;
    RotateX(dirx, diry, dirz, viewphi);
    RotateY(dirx, diry, dirz, viewtheta);
}

void getViewVec(float * dirx, float * diry, float * dirz)
{
    *dirx = 0;
    *diry = 0;
    *dirz = 1;
    RotateX(dirx, diry, dirz, viewphi);
    RotateY(dirx, diry, dirz, viewtheta);
}

void getUpVec(float * dirx, float * diry, float * dirz)
{
    *dirx = 0;
    *diry = 1;
    *dirz = 0;
    RotateX(dirx, diry, dirz, viewphi);
    RotateY(dirx, diry, dirz, viewtheta);
}

void getRightVec(float * dirx, float * diry, float * dirz)
{
    *dirx = 1;
    *diry = 0;
    *dirz = 0;
    RotateX(dirx, diry, dirz, viewphi);
    RotateY(dirx, diry, dirz, viewtheta);
}

void getInvViewVec(float * dirx, float * diry, float * dirz)
{
    *dirx = 0;
    *diry = 0;
    *dirz = 1;
    RotateY(dirx, diry, dirz, -viewtheta);
    RotateX(dirx, diry, dirz, -viewphi);
}

void getInvUpVec(float * dirx, float * diry, float * dirz)
{
    *dirx = 0;
    *diry = 1;
    *dirz = 0;
    RotateY(dirx, diry, dirz, -viewtheta);
    RotateX(dirx, diry, dirz, -viewphi);
}

void getInvRightVec(float * dirx, float * diry, float * dirz)
{
    *dirx = 1;
    *diry = 0;
    *dirz = 0;
    RotateY(dirx, diry, dirz, -viewtheta);
    RotateX(dirx, diry, dirz, -viewphi);
}

float xpos = xzsz / 2 + 0.5, ypos = ysz / 2 + 0.5, zpos = xzsz / 2 + 0.5;

voxel_t * getPointAt()
{
    float dirx, diry, dirz;
    getViewVec(&dirx, &diry, &dirz);
    return IntTrace(dirx, diry, dirz, xpos, ypos, zpos, NULL);
}

void SetPos(float x, float y, float z)
{
    int xi = (int)floor(x), yi = (int)floor(y), zi = (int)floor(z);
    if(!IsVoxelInsideArray(xi, yi, zi))
        return;
    if(*getVoxelAddr(xi, yi, zi) != emptyVoxel)
        return;
    xpos = x; ypos = y; zpos = z;
}

void moveForward(float amount)
{
    float dirx, diry, dirz;
    getViewVec(&dirx, &diry, &dirz);
    float r = sqrt(dirx * dirx + diry * diry + dirz * dirz);
    amount /= r;
    dirx *= amount;
    diry *= amount;
    dirz *= amount;
    SetPos(xpos + dirx, ypos + diry, zpos + dirz);
}

float ZBuf[YRes + 1][XRes + 1];
voxel_t * VBuf[YRes + 1][XRes + 1];

voxel_t dummyVoxel;

void initFrame()
{
    for(int y=0;y<=YRes;y++)
        for(int x=0;x<=XRes;x++)
            ZBuf[y][x] = 0;
    for(int y=0;y<=YRes;y++)
        for(int x=0;x<=XRes;x++)
            VBuf[y][x] = &dummyVoxel;
}

inline Uint32 GetColor(voxel_t * v)
{
    if(!v) return 0;
    return *v;
}

voxel_t * DrawPixel(int X, int Y)
{
    if(VBuf[Y][X] != &dummyVoxel)
        return VBuf[Y][X];
    float dirx, diry, dirz;
    getDir(&dirx, &diry, &dirz, X, Y);
    voxel_t * pv = IntTrace(dirx, diry, dirz, xpos, ypos, zpos, NULL);
    VBuf[Y][X] = pv;
    Uint32 color = GetColor(pv);
    SetPixel(X, Y, color);
    return pv;
}

void FillBlock(int minx, int maxx, int miny, int maxy, Uint32 color)
{
#if 1
    for(int y=miny;y<maxy;y++)
    {
        for(int x=minx;x<maxx;x++)
        {
            SetPixel(x, y, color);
        }
    }
#endif
}

void DrawFrameBlock(int minx, int maxx, int miny, int maxy, voxel_t * lt, voxel_t * rt, voxel_t * lb, voxel_t * rb)
{
    if(minx >= maxx || miny >= maxy)
        return;
    if(maxx - minx == 1 && maxy - miny == 1)
    {
        return;
    }
    if(lt == rt && lt == lb && lt == rb)
    {
        FillBlock(minx, maxx, miny, maxy, GetColor(lt));
        return;
    }
    if(maxx - minx > maxy - miny)
    {
        int midx = (minx + maxx) / 2;
        voxel_t * mt = DrawPixel(midx, miny);
        voxel_t * mb = DrawPixel(midx, maxy);
        DrawFrameBlock(minx, midx, miny, maxy, lt, mt, lb, mb);
        DrawFrameBlock(midx, maxx, miny, maxy, mt, rt, mb, rb);
    }
    else
    {
        int midy = (miny + maxy) / 2;
        voxel_t * lm = DrawPixel(minx, midy);
        voxel_t * rm = DrawPixel(maxx, midy);
        DrawFrameBlock(minx, maxx, miny, midy, lt, rt, lm, rm);
        DrawFrameBlock(minx, maxx, midy, maxy, lm, rm, lb, rb);
    }
}

void DrawFrameRT()
{
    DrawFrameBlock(0, XRes, 0, YRes, DrawPixel(0, 0), DrawPixel(XRes, 0), DrawPixel(0, YRes), DrawPixel(XRes, YRes));
}

const int ScanlineViewDist = 40;

#if 0
inline void DrawPlaneLine(int yi, float a, float b, float c, float d, float xa, float xb, float xc, float xd, float ya, float yb, float yc, float yd, float za, float zb, float zc, float zd)
{
    if(d == 0)
        return;
    c /= -d;
    if(c <= 0)
        return;
    a /= -d;
    b /= -d;
    float y = 1.0 - (float)yi / (float)YRes * 2.0;
    float x = -1;
    float dx = 2.0 / (float)XRes;
    float zdist = a * x + b * y + c;
    float dzdist = a * 2.0 / (float)XRes;
    for(int xi=0;xi<XRes;xi++,zdist+=dzdist,x+=dx)
    {
        if(ZBuf[yi][xi] >= zdist)
            continue;
        ZBuf[yi][xi] = zdist;
        float fz = 1 / zdist;
        float tx = (xa * x + xb * y + xc) * fz + xd;
        float ty = (ya * x + yb * y + yc) * fz + yd;
        float tz = (za * x + zb * y + zc) * fz + zd;
        if(fabs(tx - xpos) >= ScanlineViewDist || fabs(ty - ypos) >= ScanlineViewDist || fabs(tz - zpos) >= ScanlineViewDist)
        {
            VBuf[yi][xi] = NULL;
            continue;
        }
        int txi = (int)floor(tx);
        int tyi = (int)floor(ty);
        int tzi = (int)floor(tz);
        VBuf[yi][xi] = NULL;
        if(IsVoxelInsideArray(txi, tyi, tzi))
            VBuf[yi][xi] = getVoxelAddr(txi, tyi, tzi);
    }
}

void DrawFrameLine(int y)
{
    float iupx, iupy, iupz, irightx, irighty, irightz, idirx, idiry, idirz, dirx, diry, dirz;
    getViewVec(&dirx, &diry, &dirz);
    getInvViewVec(&idirx, &idiry, &idirz);
    getInvUpVec(&iupx, &iupy, &iupz);
    getInvRightVec(&irightx, &irighty, &irightz);
    for(int dx=-1;dx<=1;dx+=2)
    {
        int start;
        if(dx < 0)
            start = (int)ceil(xpos) - 1;
        else
            start = (int)floor(xpos) + 1;
        for(int x=xpos,i=0;i<ScanlineViewDist;x+=dx,i++)
        {
            DrawPlaneLine(y, )
        }
    }
}

void DrawFrameSL()
{
    for(int y=0;y<YRes;y++)
        DrawFrameLine(y);
}
#endif

int main(int argc, char ** argv)
{
    InitVoxels();
    // initialize SDL video
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Unable to init SDL: %s\n", SDL_GetError());
        return 1;
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // create a new window
#if 0
    screen = SDL_SetVideoMode(XRes, YRes, 32, SDL_FULLSCREEN);
#else
    screen = SDL_SetVideoMode(XRes, YRes, 32, 0);
#endif
    if(!screen)
    {
        printf("Unable to set video: %s\n", SDL_GetError());
        return 1;
    }

    SDL_WarpMouse(XRes / 2, YRes / 2);

    float fps = 10;
    double starttime = Timer();
    int frames = 0;

    // program main loop
    bool done = false;
    while(!done)
    {
        // message processing loop
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            // check for messages
            switch(event.type)
            {
                // exit if the window is closed
            case SDL_QUIT:
                done = true;
                break;

                // check for keypresses
            case SDL_KEYDOWN:
            {
                // exit if ESCAPE is pressed
                if(event.key.keysym.sym == SDLK_ESCAPE)
                    done = true;
                else if(event.key.keysym.sym == SDLK_DELETE)
                {
                    voxel_t * pv = getPointAt();
                    if(pv) *pv = emptyVoxel;
                }
                break;
            }
            } // end switch
        } // end of message processing

        frames++;
        double curtime = Timer();
        if(curtime - 0.1 > starttime)
        {
            fps = frames / (curtime - starttime);
            frames = 0;
            starttime = curtime;
        }

        int mousex, mousey;
        Uint8 mousebuttons = SDL_GetMouseState(&mousex, &mousey);
        SDL_WarpMouse(XRes / 2, YRes / 2);

        if(mousebuttons & SDL_BUTTON_LMASK)
            moveForward(2 / fps);

        mousex -= XRes / 2;
        mousey -= YRes / 2;
        viewtheta = fmod(viewtheta + (float)mousex / 100, M_PI * 2);
        viewphi -= (float)mousey / 100;
        if(viewphi < -M_PI / 2)
            viewphi = -M_PI / 2;
        else if(viewphi > M_PI / 2)
            viewphi = M_PI / 2;

        // DRAWING STARTS HERE

        initFrame();

        SDL_LockSurface(screen);

        DrawFrameRT();

        SDL_UnlockSurface(screen);

        // DRAWING ENDS HERE

        // finally, update the screen :)
        SDL_Flip(screen);
    } // end main loop

    // all is well ;)
    printf("Exited cleanly\n");
    return 0;
}
