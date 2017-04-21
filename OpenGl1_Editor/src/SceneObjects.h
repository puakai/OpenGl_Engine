#ifndef SCENE_H
#define SCENE_H

#include "Engine.h"

typedef unsigned char COMPONENT_TYPE;
typedef unsigned char DRAWORDER;
#define DRAWORDER_NOT 0x00
#define DRAWORDER_SOLID 0x01
#define DRAWORDER_TRANSPARENT 0x02
#define DRAWORDER_OVERLAY 0x04
#define DRAWORDER_LIGHT 0x08
class Component : public Object {
public:
	Component(string name, COMPONENT_TYPE t, DRAWORDER drawOrder = 0x00, SceneObject* o = nullptr, vector<COMPONENT_TYPE> dep = {});
	virtual  ~Component() {}

	const COMPONENT_TYPE componentType = 0;
	const DRAWORDER drawOrder;
	bool active;
	SceneObject* object;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, ofstream* stream);
	friend class Scene;
	friend class Editor;
	friend class EB_Viewer;
	friend class EB_Inspector;
	friend class SceneObject;
	friend void EBI_DrawObj(Vec4 v, Editor* editor, EB_Inspector* b, SceneObject* o);
	friend bool DrawComponentHeader(Editor* e, Vec4 v, uint pos, Component* c);
	friend void DrawSceneObjectsOpaque(EB_Viewer* ebv, vector<SceneObject*> oo), DrawSceneObjectsGizmos(EB_Viewer* ebv, vector<SceneObject*> oo), DrawSceneObjectsTrans(EB_Viewer* ebv, vector<SceneObject*> oo);

protected:
	vector<COMPONENT_TYPE> dependancies;
	vector<Component*> dependacyPointers;

	//bool serializable;
	//vector<pair<void*, void*>> serializedValues;

	bool _expanded;

	static COMPONENT_TYPE Name2Type(string nm);

	virtual void LoadDefaultValues() {} //also loads assets
	virtual void DrawEditor(EB_Viewer* ebv) {} //trs matrix not applied, apply before calling
	virtual void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) = 0;
	//virtual void DrawGameCamera() {}
	virtual void Serialize(Editor* e, ofstream* stream) = 0;
	virtual void Refresh() {}
};

class Transform : public Object {
public:

	SceneObject* object;
	Vec3 position, worldPosition(), forward();
	Quat rotation, worldRotation();
	const Vec3& eulerRotation() const { return _eulerRotation;  }
	void eulerRotation(Vec3 r);
	Vec3 scale;

	Transform* Translate(float x, float y, float z) { return Translate(Vec3(x, y, z)); }
	Transform* Rotate(float x, float y, float z) { return Rotate(Vec3(x, y, z)); }
	Transform* Scale(float x, float y, float z) { return Scale(Vec3(x, y, z)); }
	Transform* Translate(Vec3 v), *Rotate(Vec3 v), *Scale(Vec3 v);

	friend class SceneObject;
private:
	//never call Transform constructor directly. Call SceneObject().transform instead.
	Transform(SceneObject* sc, Vec3 pos, Quat rot, Vec3 scl);
	Vec3 _eulerRotation;
	void _UpdateQuat();
};

class Mesh : public AssetObject {
public:
	//Mesh(); //until i figure out normal recalc algorithm
	bool loaded;

	vector<Vec3> vertices;
	vector<Vec3> normals;
	vector<int> triangles;
	vector<Vec2> uv0, uv1;

	uint vertexCount, triangleCount, materialCount;

	friend int main(int argc, char **argv);
	friend class Editor;
	friend class MeshFilter;
	friend class MeshRenderer;
	friend class AssetManager;
protected:
	Mesh(Editor* e, int i);
	Mesh(ifstream& strm, uint offset);
	Mesh(string path);

	static bool ParseBlend(Editor* e, string s);
	vector<vector<int>> _matTriangles;
	//void Draw(Material* mat);
	//void Load();
};

class FCurve_Key {
public:
	FCurve_Key(Vec2 a, Vec2 b, Vec2 c) : point(a), left(b), right(c) {}
	Vec2 point, left, right;
};

enum FCurveType : byte {
	FC_Undef = 0x00,
	FC_BoneLoc_X = 0x10,
	FC_BoneLoc_Y, FC_BoneLoc_Z,
	FC_BoneQuat_W, FC_BoneQuat_X, FC_BoneQuat_Y, FC_BoneQuat_Z,
	FC_BoneScl_X, FC_BoneScl_Y, FC_BoneScl_Z,
	FC_ShapeKey = 0x20,
	FC_Location_X = 0xf0,
	FC_Location_Y, FC_Location_Z,
	FC_Rotation_X, FC_Rotation_Y, FC_Rotation_Z,
	FC_Scale_X = 0xf7,
	FC_Scale_Y, FC_Scale_Z,
};
class FCurve {
public:
	FCurve() : type(FC_Undef), name(""), keys(), keyCount(0), startTime(0), endTime(0) {}
	FCurveType type;
	string name;
	vector<FCurve_Key> keys;
	byte keyCount;
	float startTime, endTime;
	float Eval(float t, float repeat = false);
};

class AnimClip : public AssetObject {
	vector<FCurve> curves;
	uint curveLength;

	friend class Editor;
	friend class AssetManager;
protected:
	AnimClip(Editor* e, int i);
	AnimClip(ifstream& strm, uint offset);
	AnimClip(string path);
};

class AnimFrameItem {
public:
	AnimFrameItem();
	friend class Animator;
	friend class Anim_State;
protected:
	byte type;
	string name;
	Vec3 transform, scale;
	Quat rotation;
};

class Anim_State {
public:
	Anim_State();
	friend class Animator;
protected:
	bool blend;
	float speed, length, time;
	float Eval(); //increments time automatically

	AnimClip* clip;
	ASSETID _clip;
	Vec2 editorPos;

	vector<AnimClip*> blendClips;
	vector<ASSETID> _blendClips;
	vector<float> blendOffsets;
};

class Anim_Transition {
public:
	Anim_Transition();
	friend class Animator;
protected:
	bool canInterrupt;
	float length, time;

};

class Animator : public AssetObject {

	friend class EB_AnimEditor;
	friend class Editor;
protected:
	Animator();
	Animator(string s);
	Animator(ifstream& stream, uint offset);
	uint activeState, nextState;
	float stateTransition;

	vector <Anim_State> states;
	vector <Anim_Transition> transitions;

	void Save(string s);
};

class RenderTexture : public AssetObject {
public:
	uint width, height;
};

enum TEX_FILTERING : byte {
	TEX_FILTER_POINT = 0x00,
	TEX_FILTER_BILINEAR,
	TEX_FILTER_TRILINEAR
};

class Texture : public AssetObject {
public:
	Texture(const string& path);
	Texture(const string& path, bool mipmap);
	Texture(const string& path, bool mipmap, TEX_FILTERING filter, byte aniso);
	//Texture(ifstream& stream, long pos);
	~Texture(){ glDeleteTextures(1, &pointer); }
	bool loaded;
	unsigned int width, height;
	GLuint pointer;
	friend int main(int argc, char **argv);
	friend class Editor;
	friend class EB_Inspector;
	friend class AssetManager;
	friend void EBI_DrawAss_Tex(Vec4 v, Editor* editor, EB_Inspector* b, float &off);
private:
	Texture(int i, Editor* e); //for caches
	Texture(ifstream& strm, uint offset);
	static void _ReadStrm(Texture* tex, ifstream& strm, byte& chn, GLenum& rgb, GLenum& rgba);
	byte _aniso = 0;
	TEX_FILTERING _filter = TEX_FILTER_POINT;
	bool _mipmap = true, _repeat = false;
	static bool Parse(Editor* e, string path);
	void _ApplyPrefs(const string& p);
};

class Background : public AssetObject {
public:
	//Background() : AssetObject(ASSETTYPE_HDRI){}
	Background(const string& path);

	bool loaded;
	unsigned int width, height;
	GLuint pointer;
	friend int main(int argc, char **argv);
	friend class Editor;
	friend class AssetManager;
private:
	Background(int i, Editor* editor);
	Background(ifstream& strm, uint offset);
	static bool Parse(string path);
	static vector<float> Downsample(vector<float>&, uint, uint, uint&, uint&);
};

#define COMP_UNDEF 0x00

#define COMP_CAM 0x01
enum CAM_CLEARTYPE : byte {
	CAM_CLEAR_NONE,
	CAM_CLEAR_COLOR,
	CAM_CLEAR_DEPTH,
	CAM_CLEAR_SKY
};

enum GBUFFERS {
	GBUFFER_DIFFUSE,
	GBUFFER_NORMAL,
	GBUFFER_SPEC_GLOSS,
	GBUFFER_Z,
	GBUFFER_NUM_TEXTURES
};

class Camera : public Component {
public:
	Camera();

	bool ortographic;
	float fov, orthoSize;
	Rect screenPos;
	CAM_CLEARTYPE clearType;
	Vec4 clearColor;
	float nearClip;
	float farClip;
	RenderTexture* targetRT;


	void Render(RenderTexture* target = nullptr);

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, ofstream* stream);
	friend void Deserialize(ifstream& stream, SceneObject* obj);
	friend class EB_Viewer;
	friend class EB_Inspector;
protected:
	Camera(ifstream& stream, SceneObject* o, long pos = -1);

	void RenderLights();
	void DumpBuffers();
	void _RenderSky(glm::mat4 ip);
	void _DrawLights(vector<SceneObject*> oo, glm::mat4& ip);
	void _DoDrawLight_Point(Light* l, glm::mat4& ip);
	void _DoDrawLight_Spot(Light* l, glm::mat4& ip);

	Vec3 camVerts[6];
	static int camVertsIds[19];
	GLuint d_fbo, d_texs[3], d_depthTex;
	GLuint d_skyProgram, d_pLightProgram, d_sLightProgram;

	static const Vec2 screenRectVerts[];
	static const int screenRectIndices[];

	int _tarRT;
	
	void ApplyGL();

	void UpdateCamVerts();
	void InitGBuffer();
	void DrawEditor(EB_Viewer* ebv) override;
	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, ofstream* stream) override;

	static void _SetClear0(EditorBlock* b), _SetClear1(EditorBlock* b), _SetClear2(EditorBlock* b), _SetClear3(EditorBlock* b);
};

#define COMP_MFT 0x02
class MeshFilter : public Component {
public:
	MeshFilter();
	
	Mesh* mesh;
	//void LoadDefaultValues() override;

	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, ofstream* stream) override;
	
	friend class Editor;
	friend void LoadMeshMeta(vector<SceneObject*>& os, string& path);
	friend void Deserialize(ifstream& stream, SceneObject* obj);
protected:
	MeshFilter(ifstream& stream, SceneObject* o, long pos = -1);

	void SetMesh(int i);
	static void _UpdateMesh(void* i);
	ASSETID _mesh;
};

#define COMP_MRD 0x10
class MeshRenderer : public Component {
public:
	MeshRenderer();
	vector<Material*> materials;

	void DrawEditor(EB_Viewer* ebv) override;
	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;

	void Serialize(Editor* e, ofstream* stream) override;
	void Refresh() override;

	friend class Editor;
	friend void Deserialize(ifstream& stream, SceneObject* obj);
	friend void DrawSceneObjectsOpaque(vector<SceneObject*> oo);
protected:
	MeshRenderer(ifstream& stream, SceneObject* o, long pos = -1);

	void DrawDeferred();

	vector<ASSETID> _materials;
	static void _UpdateMat(void* i);
	static void _UpdateTex(void* i);
};

#define COMP_TRD 0x11
class TextureRenderer : public Component {
public:
	TextureRenderer(): _texture(-1), Component("Texture Renderer", COMP_TRD, DRAWORDER_OVERLAY) {}
	
	Texture* texture;

	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, ofstream* stream) override;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, ofstream* stream);
	friend void Deserialize(ifstream& stream, SceneObject* obj);
protected:
	TextureRenderer(ifstream& stream, SceneObject* o, long pos = -1);
	int _texture;
};

#define COMP_SRD 0x12
class SkinnedMeshRenderer : public Component {
public:
	SkinnedMeshRenderer();

	vector<Material*> materials;
	Animator* anim;

	void DrawEditor(EB_Viewer* ebv) override;
	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;

	void Serialize(Editor* e, ofstream* stream) override;
	void Refresh() override;

	friend class Editor;
	friend void Deserialize(ifstream& stream, SceneObject* obj);
	friend void DrawSceneObjectsOpaque(vector<SceneObject*> oo);
protected:
	SkinnedMeshRenderer(ifstream& stream, SceneObject* o, long pos = -1);

	void DrawDeferred();

	vector<ASSETID> _materials;
	static void _UpdateMat(void* i);
	static void _UpdateTex(void* i);
};

enum LIGHTTYPE : byte {
	LIGHTTYPE_POINT,
	LIGHTTYPE_DIRECTIONAL,
	LIGHTTYPE_SPOT,
};
#define COMP_LHT 0x20
#define LIGHT_POINT_MINSTR 0.01f
class Light : public Component {
public:
	Light() : Component("Light", COMP_LHT, DRAWORDER_LIGHT), _lightType(LIGHTTYPE_POINT), intensity(1), color(white()), angle(45), minDist(0), maxDist(30), drawShadow(false), shadowBias(0.1f), shadowStrength(1) {}
	LIGHTTYPE lightType() { return _lightType; }

	float intensity;
	Vec4 color;
	float angle;
	float minDist, maxDist;
	bool drawShadow;
	float shadowBias, shadowStrength;

	void DrawEditor(EB_Viewer* ebv) override;
	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;
	void DrawShadowMap();
	void Serialize(Editor* e, ofstream* stream) override;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, ofstream* stream);
	friend void Deserialize(ifstream& stream, SceneObject* obj);
	friend class Camera;
protected:
	LIGHTTYPE _lightType;
	Light(ifstream& stream, SceneObject* o, long pos = -1);
	glm::mat4 _shadowMatrix;

	void InitShadow(), CalcShadowMatrix();
	static GLuint _shadowFbo, _shadowMap;
};

#define COMP_ARM 0x30
class ArmatureBone {
public:
	SceneObject* obj;
	//transform is local
	Vec3 position, restPosition;
	Quat rotation, restRotation;
	Vec3 scale, restScale;
};
class Armature : public Component {
public:
	Armature() : _anim(-1), Component("Armature", COMP_ARM, DRAWORDER_OVERLAY) {}

	Animator* anim;

	Vec3 position, restPosition;
	Quat rotation, restRotation;
	Vec3 scale, restScale;

	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override;
	void Serialize(Editor* e, ofstream* stream) override;

	friend int main(int argc, char **argv);
	friend void Serialize(Editor* e, SceneObject* o, ofstream* stream);
	friend void Deserialize(ifstream& stream, SceneObject* obj);
protected:
	Armature(ifstream& stream, SceneObject* o, long pos = -1);
	ASSETID _anim;
};

#define COMP_SCR 0xff
enum SCR_VARTYPE : byte {
	SCR_VAR_UNDEF = 0U,
	SCR_VAR_INT, SCR_VAR_UINT,
	SCR_VAR_FLOAT,
	SCR_VAR_V2, SCR_VAR_V3,
	SCR_VAR_STRING = 16U,
	SCR_VAR_TEXTURE,
	SCR_VAR_COMPREF,
	SCR_VAR_COMMENT = 255U
};
class SceneScript : public Component {
public:
	~SceneScript();

	virtual void Start() {}
	virtual void Update() {}
	virtual void LateUpdate() {}
	virtual void Paint() {}

	static void Parse(string s, Editor* e);

	//bool ReferencingObject(Object* o) override;
	friend class Editor;
	friend class EB_Viewer;
	friend void Deserialize(ifstream& stream, SceneObject* obj);
protected:
	SceneScript(Editor* e, ASSETID id);
	SceneScript(ifstream& strm, SceneObject* o);
	SceneScript() : Component("", COMP_SCR, DRAWORDER_NOT) {}
	
	ASSETID _script;
	vector<pair<string, pair<SCR_VARTYPE, void*>>> _vals;

	void DrawInspector(Editor* e, Component*& c, Vec4 v, uint& pos) override; //we want c to be null if deleted
	void Serialize(Editor* e, ofstream* stream) override;
};

class SceneObject : public Object {
public:
	SceneObject();
	SceneObject(string s);
	SceneObject(Vec3 pos, Quat rot, Vec3 scale);
	SceneObject(string s, Vec3 pos, Quat rot, Vec3 scale);
	~SceneObject();
	bool active;
	int childCount;
	Transform transform;

	void Enable(), Disable();
	void Enable(bool enableAll), Disable(bool disableAll);

	SceneObject* parent;
	vector<SceneObject*> children;

	SceneObject* AddChild(SceneObject* child);
	SceneObject* GetChild(int i) { return children[i]; }
	void RemoveChild(SceneObject* o);
	Component* AddComponent(Component* c);
	
	Component* GetComponent(COMPONENT_TYPE type);
	template<class T> T* GetComponent() {
		//static_assert(is_base_of(Component, T), "GetComponent requires a component type!");
		(void)static_cast<Component*>((T*)0);
		for (Component* cc : _components)
		{
			T* xx = dynamic_cast<T*>(cc);
			if (xx != nullptr) {
			//if (typeid(&cc) == typeid(T)) {
				return xx;
			}
		}
		return nullptr;
	}
	void RemoveComponent(Component*& c);

	bool _expanded;
	int _componentCount;
	vector<Component*> _components;

	friend class MeshFilter;
	friend class Scene;
protected:
	void Refresh();
};

#endif