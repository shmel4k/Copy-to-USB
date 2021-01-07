#include "mainwindow.h"
#include <QWidget>

#include <boost/iostreams/device/mapped_file.hpp>


MainWindow::MainWindow(QWidget *parent)
  : QWidget(parent)
  , m_homedir(QString())
  , fileName1(QString())
  , fileName2(QString())
  , fileLineEdit1(new QLineEdit(this))
  , m_button_file1(new QPushButton("...", this))
  , m_usbComboBox(new QComboBox)
  , m_button_umount(new QPushButton("Unmount", this))
  , m_button_refreshUSB(new QPushButton("Refresh USB list", this))
  , progressBar(new QProgressBar(this))
  , compareCheckBox(new QCheckBox("Check the files for identity after coping"))
  , m_button_start(new QPushButton("Start", this))
  , m_button_quit(new QPushButton("Quit", this))
{
  const char *homedir;
  if ((homedir = getenv("HOME")) == NULL)
  {
      homedir = getpwuid(getuid())->pw_dir;
  }
  m_homedir = homedir;

  progressBar->setRange(0, 100);
  progressBar->setValue(0);

  m_button_start->setDisabled(true);

  connect(fileLineEdit1, &QLineEdit::textChanged, this, &MainWindow::enableStartButton);
  connect(m_button_file1, SIGNAL (clicked()), this, SLOT (handleSelectFile1()));
  connect(m_button_umount, SIGNAL (clicked()), this, SLOT (handleUmount()));
  connect(m_button_start, SIGNAL (clicked()), this, SLOT (handleStart()));
  connect(m_button_quit, SIGNAL (clicked()), QApplication::instance(), SLOT (quit()));
  connect(this, SIGNAL(valueChanged(int)), progressBar, SLOT(setValue(int)));
  connect(m_usbComboBox, SIGNAL(activated(int)), this, SLOT(usbChanged(int)));
  connect(m_button_refreshUSB, SIGNAL(clicked()), this, SLOT(usbRefresh()));


  QGroupBox *fileGroup1 = new QGroupBox(tr("Source file"));
  QLabel *gr1Label = new QLabel(tr("File:"));
  QGridLayout *fileGroup1Layout = new QGridLayout;
  fileGroup1Layout->addWidget(gr1Label, 0, 0);
  fileGroup1Layout->addWidget(m_button_file1, 0, 1);
  fileGroup1Layout->addWidget(fileLineEdit1, 1, 0, 1, 2);
  fileGroup1->setLayout(fileGroup1Layout);

  defineUSB();
  for(auto x : m_usb)
  {
      m_usbComboBox->addItem(x.second);
  }

  QGroupBox *fileGroup2 = new QGroupBox(tr("Destination"));
  QLabel *gr2Label = new QLabel(tr("USB:"));
  QGridLayout *fileGroup2Layout = new QGridLayout;
  fileGroup2Layout->addWidget(gr2Label, 0, 0);
  fileGroup2Layout->addWidget(m_usbComboBox, 0, 1);
  fileGroup2Layout->addWidget(m_button_umount, 0, 2);
  fileGroup2Layout->addWidget(m_button_refreshUSB, 2, 1);
  fileGroup2->setLayout(fileGroup2Layout);

  QGroupBox *progressGroup = new QGroupBox(tr("Progress"));
  QGridLayout *progressLayout = new QGridLayout;
  progressLayout->addWidget(progressBar, 0, 0, 1, 2);
  progressGroup->setLayout(progressLayout);

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(fileGroup1, 0, 0);
  layout->addWidget(fileGroup2, 0, 1);
  layout->addWidget(progressGroup, 1, 0, 1, 2);
  layout->addWidget(compareCheckBox, 2, 0, 1, 2);
  layout->addWidget(m_button_start, 3, 0);
  layout->addWidget(m_button_quit, 3, 1);
  setLayout(layout);

  setWindowTitle(tr("Copy file to USB"));

}

MainWindow::~MainWindow()
{
}

void MainWindow::handleSelectFile1()
{
  fileName1 = QFileDialog::getOpenFileName(this, tr("Open File"), m_homedir);
  fileLineEdit1->setText(fileName1);
}

void MainWindow::usbChanged(int index)
{
  fileName2 = m_usb[index].first;
}

void MainWindow::usbRefresh()
{
  defineUSB();
  m_usbComboBox->clear();
  for(auto x : m_usb)
  {
      m_usbComboBox->addItem(x.second);
  }

  int index = m_usbComboBox->currentIndex();
  if (-1 == index)
  { // an empty combo box or a combo box in which no current item is set
      m_button_umount->setDisabled(true);
      m_button_start->setDisabled(true);
  }
  else
  {
      m_button_umount->setEnabled(true);
      m_button_start->setEnabled(true);
  }
}

void MainWindow::handleUmount()
{
  const int index = m_usbComboBox->currentIndex();
  if (index == -1)
  { // an empty combo box or a combo box in which no current item is set
      return;
  }

  QByteArray ba = m_usb[index].first.toLatin1();
  const char *c_str = ba.data();
  int err;
  if (-1 == (err = umount(c_str)))
  {
     QMessageBox(QMessageBox::Critical, QString("Unmounting USB..."), "Operation fails").exec();
  }

  usbRefresh();
}

void MainWindow::enableStartButton()
{
  fileName1 = fileLineEdit1->text();
  m_button_start->setEnabled(!fileName1.isEmpty());
  valueChanged(0);
}

void MainWindow::handleStart()
{
  QByteArray ba = fileName1.toLatin1();
  const char *c_str1 = ba.data();
  std::string name1(c_str1);

  ba = fileName2.toLatin1();
  const char *c_str2 = ba.data();
  std::string name2(c_str2);

  // USB TARGET is without tailing slash,
  // so add it here + file_name
  std::string str;
  std::stringstream ss(name1);
  while(getline(ss, str, '/'));
  name2 += "/" + str;

  if (name1 == name2)
  {
     std::string serr = "Copying file to itself is not supported";
     QMessageBox(QMessageBox::Warning, QString("Wrong parameters"), QString::fromStdString(serr)).exec();
     return;
  }

  std::ifstream istr(name1, std::ios::in | std::ios::binary);
  if (istr.rdstate())
  {
     std::string serr = "Can't open file " + name1;
     QMessageBox(QMessageBox::Critical, QString("Error"), QString::fromStdString(serr)).exec();
     return;
  }

  // file size
  const int64_t fsize = istr.rdbuf()->in_avail();
  if (fsize <= 0)
  {
     std::string serr = "Input file has wrong size: " + std::to_string(fsize);
     QMessageBox(QMessageBox::Critical, QString("Error"), QString::fromStdString(serr)).exec();
     istr.close();
     return;
  }

  std::ofstream ostr(name2, std::ios::out | std::ios::binary);
  if (ostr.rdstate())
  {
     std::string serr = "Can't open file " + name2;
     QMessageBox(QMessageBox::Critical, QString("Error"), QString::fromStdString(serr)).exec();
     istr.close();
     return;
  }

  const int64_t space = get_free_size(name2.data());
  if (fsize >= space)
  {
     std::string serr = "Not enough space (" + std::to_string(space>>20) + "M)";
     QMessageBox msg(QMessageBox::Warning, QString("Disk space"), QString::fromStdString(serr));
     msg.exec();
     if (-1 == std::remove(name2.data()))
     {
        serr = "Error while clearing out file " + name2;
        msg.setIcon(QMessageBox::Critical);
        msg.setText(QString::fromStdString(serr));
        msg.exec();
     }
     istr.close();
     ostr.close();
     return;
  }

  size_t bsize = 1<<13;
  char *buf = new char[bsize];
  int val = 0, newVal;
  valueChanged(val);
  m_button_quit->setDisabled(true);
  m_button_umount->setDisabled(true);
  m_button_refreshUSB->setDisabled(true);
  while(istr.read(buf,bsize).gcount() > 0)
  {
     ostr.write(buf, istr.gcount());
     if (val != (newVal = 100 * ostr.tellp() / fsize))
     {
        val = newVal;
        valueChanged(val);
     }
  }
  delete[] buf;

  ostr.close();
  m_button_quit->setEnabled(true);
  m_button_umount->setEnabled(true);
  m_button_refreshUSB->setEnabled(true);
  istr.close();

  if (compareCheckBox->isChecked())
  {
      boost::iostreams::mapped_file_source f1(name1);
      boost::iostreams::mapped_file_source f2(name2);

      bool res = f1.size() == f2.size() && std::equal(f1.data(), f1.data() + f1.size(), f2.data());
      std::string text = res ? "These two files are identical"
                             : "Error: these two files are not the same";
      QMessageBox(res ? QMessageBox::Information : QMessageBox::Warning,
                  QString("Identity verification"), QString::fromStdString(text)).exec();
  }
}

int64_t MainWindow::get_free_size(const char *path)
{
  int64_t res;
  struct statvfs buf;

  if (-1 == (res = statvfs(path, &buf)))
  {
      std::string serr = "statvfs fails";
      QMessageBox(QMessageBox::Critical, QString("Error"), QString::fromStdString(serr)).exec();
  }
  else
  {
      res = buf.f_bsize * buf.f_bfree;
  }
  return res;
}

void MainWindow::defineUSB(void)
{
  m_usb.clear();

  FILE *f = popen("findmnt -r | grep /dev/sd | awk '{ print $1; }'", "r");
  if (NULL != f)
  {
      std::string str;
      std::string target;
      std::string label;

      int MAXLEN = 132;
      char buf[MAXLEN];
      while(fgets(buf, MAXLEN , f))
      {
         target = buf;
         if (target.back() == '\n')
         {
            target.pop_back();
         }

         std::stringstream ss(target);

         label.clear();
         while(std::getline(ss, str, '/'))
         {
            label = str;
         }
         m_usb.push_back(std::pair<QString,QString>(QString::fromStdString(target), QString::fromStdString(label)));
      }

      pclose(f);
  }

  return;
}
