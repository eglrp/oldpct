#include "MainWindow.h"
#include <QtWidgets/QApplication>
#include <vtkOutputWindow.h>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

    // ����vtk���洰�ڲ���ʾ
    vtkOutputWindow::SetGlobalWarningDisplay(0);

    // ��ʾ������
	MainWindow w;
	w.show();
	
	return a.exec();
}
