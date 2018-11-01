#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"
#include "QRendView.h"
#include <QtGui>
#include <QMainWindow>
#include <QActionGroup>
#include <QSpinBox>
#include <QSlider>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <localTypes.h>
#include <QPushButton>
#include <CloudEditorWidgetEx.h>

/**
* ����������
* @author ��ΰ��
* �����˲˵�����������������ͼ���¼�����
*/
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = Q_NULLPTR);
	~MainWindow();
	
    // ����xyz�ļ�
    void LoadXyzFile(const std::string &file);

    // ����pcd�ļ�
	void loadFilePCD(const std::string &filename);
	
    // ����las�ļ�
    void loadFileLAS(const std::string &filename);

    // ����ply�����ļ�
    void loadPlyFile(const std::string & file);
	
    // дlas�ļ�
    void WriteLasFile(const QString& file);
	
    // дpcd�ļ�
    void WritePcdFile(const QString& file);
 	
    // дxyz�ļ�
    void WriteXyzFile(const QString& file);

    // дply�ļ�
    void WritePlyFile(const QString & file);


private:
    void Offset_center();

 public slots:
 // ���ļ�
 void OpenFile();

 // д�ļ�
 void WriteFile();
 
 // ����
 void about();

 // ����
 void help();


private:
    // �����źŲ�
    void InitUiConnect();

    Ui::MainWindow ui;

    // �༭��ť
    QPushButton* menu_cornerbtn_;
};
