#include "level/tile/GrassTile.hpp"
#include "level/tile/BushTile.hpp"
#include "level/tile/LiquidTile.hpp"
#include "level/tile/CalmLiquidTile.hpp"
#include "level/tile/FallingTile.hpp"
#include "level/tile/LogTile.hpp"
#include "level/tile/TransparentTile.hpp"
#include "level/tile/SpongeTile.hpp"
#include "level/tile/GlassTile.hpp"
#include "level/tile/Tile.hpp"

std::array<Tile*, 256> Tile::tiles = {nullptr};
const Tile* Tile::empty = nullptr;

static Tile rockTile(1, 1);
static GrassTile grassTile(2);
static Tile dirtTile(3, 3);
static Tile cobbleTile(4, 5);
static Tile woodTile(5, 2);
static Bush bushTile(6);
static Tile unbreakableTile(7, 17);
static LiquidTile waterTile(8, 1);
static CalmLiquidTile calmWaterTile(9, 1);
static LiquidTile lavaTile(10, 2);
static CalmLiquidTile calmLavaTile(11, 2);
static FallingTile gravelTile(12, 19);
static FallingTile sandTile(13, 18);
static LogTile logTile(14);
static TransparentTile leavesTile(15, 22);
static Tile goldOreTile(16, 32);
static Tile ironOreTile(17, 33);
static Tile coalOreTile(18, 34);
static SpongeTile spongeTile(19);
static GlassTile glassTile(20);

const Tile* Tile::rock = &rockTile;
const Tile* Tile::grass = &grassTile;
const Tile* Tile::dirt = &dirtTile;
const Tile* Tile::cobblestone = &cobbleTile;
const Tile* Tile::wood = &woodTile;
const Tile* Tile::bush = &bushTile;
const Tile* Tile::unbreakable = &unbreakableTile;
const Tile* Tile::water = &waterTile;
const Tile* Tile::calmWater = &calmWaterTile;
const Tile* Tile::lava = &lavaTile;
const Tile* Tile::calmLava = &calmLavaTile;
const Tile* Tile::gravel = &gravelTile;
const Tile* Tile::sand = &sandTile;
const Tile* Tile::log = &logTile;
const Tile* Tile::leaves = &leavesTile;
const Tile* Tile::goldOre = &goldOreTile;
const Tile* Tile::ironOre = &ironOreTile;
const Tile* Tile::coalOre = &coalOreTile;
const Tile* Tile::sponge = &spongeTile;
const Tile* Tile::glass = &glassTile;

Tile::Tile(int id) {
    tiles[id] = this;
    this->id = id;
    this->setShape(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    this->textureId = 0;
}

Tile::Tile(int id, int texture) : Tile(id) {
    this->textureId = texture;
}

Tile::~Tile() {}

bool Tile::shouldRenderFace(Level* level, int x, int y, int z, int layer, int face) {
    if (layer == 1) {
        return false;
    } else {
        return !level->isSolidTile(x, y, z);
    }
}

int Tile::getTexture(int face) {
    return this->textureId;
}

void Tile::setShape(float x0, float y0, float z0, float x1, float y1, float z1) {
    this->minX = x0;
    this->minY = y0;
    this->minZ = z0;
    this->maxX = x1;
    this->maxY = y1;
    this->maxZ = z1;
}

float Tile::getBrightness(Level* level, int x, int y, int z) {
    return level->getBrightness(x, y, z);
}


void Tile::neighborChanged(Level* level, int x, int y, int z, int type) {
    // no implementation
}

void Tile::tick(Level* level, int x, int y, int z, Random* random) {
    // no implementation
}

bool Tile::mayPick() {
    return true;
}

bool Tile::isSolid() {
    return true;
}

bool Tile::blocksLight() {
    return true;
}

int Tile::getLiquidType() {
    return 0;
}

bool Tile::isCalmLiquid() {
    return false;
}

void Tile::onBlockAdded(Level* level, int x, int y, int z) {
    // No implementation
}