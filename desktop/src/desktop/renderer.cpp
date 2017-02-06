#include "renderer.h"
#include "mesh.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QTimer>

#include <cstdio>

Mesh *mesh;
QOpenGLVertexArrayObject *vao;
QOpenGLBuffer *vbo;
QOpenGLShaderProgram *shr;
QOpenGLTexture *tex;

float angle;
QMatrix4x4 proj;
QMatrix4x4 model;

QTimer *timer;

Renderer::Renderer(QWidget *parent) : QOpenGLWidget(parent), logger(nullptr)
{
	logger = new QOpenGLDebugLogger(this);
	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &Renderer::onTimer);
	timer->start(30);
}

Renderer::~Renderer()
{}

void Renderer::debugLogMessage(const QOpenGLDebugMessage &msg)
{
	qDebug() << msg.message();
}

void Renderer::initializeGL()
{
	initializeOpenGLFunctions();
	logger->initialize();
	connect(logger, &QOpenGLDebugLogger::messageLogged, this, &Renderer::debugLogMessage);
	logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);

	{ // load the mesh
		FILE *f = fopen("data/4.bin", "rb");
		if (!f)
			throw "failed opening file";
		fseek(f, 0, SEEK_END);
		size_t len = ftell(f);
		fseek(f, 0, SEEK_SET);
		char *mem = (char*)malloc(len);
		if (fread(mem, len, 1, f) != 1)
			throw "failed reading file";
		fclose(f);
		mesh = new Mesh(mem);
		free(mem);

		//model.translate(-(mesh->subMeshes[0]->boxMin + mesh->subMeshes[0]->boxMax) * 0.5);
	}

	{ // prepare the mesh
		vao = new QOpenGLVertexArrayObject();
		vao->create();
		vao->bind();
		vbo = new QOpenGLBuffer();
		vbo->create();
		vbo->bind();
		vbo->allocate(mesh->subMeshes[0]->faces.data(), mesh->subMeshes[0]->faces.size() * sizeof(SubMesh::Face));
		// vertices
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SubMesh::Face::Vertex), (void*)0);
		// uvs
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SubMesh::Face::Vertex), (void*)12);
	}

	{ // load texture
		tex = new QOpenGLTexture(QImage("data/melown-logo.png"));
	}

	{ // load the shader
		shr = new QOpenGLShaderProgram();
		shr->create();
		shr->addShaderFromSourceFile(QOpenGLShader::Fragment, "data/shaders/a-fragment.glsl");
		shr->addShaderFromSourceFile(QOpenGLShader::Vertex, "data/shaders/a-vertex.glsl");
		shr->link();
	}
}

void Renderer::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	proj.perspective(45.f, w / (float)h, 0.01, 10);
}

void Renderer::paintGL()
{
	glClearColor(0.1, 0.1, 0.1, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);

	QMatrix4x4 view;
	view.lookAt(QVector3D(cos(angle), 0, sin(angle)) * 5, QVector3D(), QVector3D(0, 1, 0));
	QMatrix4x4 mvp = proj * view * model;

	vao->bind();
	tex->bind();
	shr->bind();
	shr->setUniformValue(0, mvp);
	glDrawArrays(GL_TRIANGLES, 0, 3 * mesh->subMeshes[0]->faces.size());
}

void Renderer::onTimer()
{
	angle += 5e-3;
	this->update();
}

