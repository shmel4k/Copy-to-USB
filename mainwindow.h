#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QProgressBar>
#include <QPushButton>
#include <QFileDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QWidget>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <iostream>
#include <istream>
#include <fstream>
#include <streambuf>

#include <string>
#include <vector>
#include <sstream>

#include <sys/statvfs.h>
#include <sys/mount.h>

#include <cstdio>
#include <cerrno>

class MainWindow : public QWidget
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

public slots:
  void handleSelectFile1();
  void handleUmount();
  void enableStartButton();
  void handleStart();
  void usbChanged(int);
  void usbRefresh(void);

private:
  QString m_homedir;
  QString fileName1;
  QString fileName2;
  QLineEdit *fileLineEdit1;
  QPushButton *m_button_file1;
  QComboBox *m_usbComboBox;
  QPushButton *m_button_umount;
  QPushButton *m_button_refreshUSB;
  QProgressBar *progressBar;
  QCheckBox *compareCheckBox;
  QPushButton *m_button_start;
  QPushButton *m_button_quit;
  std::vector<std::pair<QString, QString>> m_usb;

  int64_t get_free_size(const char *);
  void defineUSB(void);

Q_SIGNALS:
  void valueChanged(int);


};
#endif // MAINWINDOW_H
