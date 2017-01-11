#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QFileDialog"
#include "QMessageBox"
#include <fstream>
#include <assert.h>
#include <iostream>
#include <cmath>

const std::string UNSURE_FILE = ":File/images/unsure.png";
const std::string YES_FILE = ":File/images/yes.gif";
const std::string NO_FILE = ":File/images/no.gif";
const std::string UNKNOWN_FILE = ":File/images/unknown.png";

std::string get_label_image(AnnoState state) {
    std::string show_image_name = UNKNOWN_FILE;
    switch (state) {
    case AnnoState::UNKNOWN:
        show_image_name = UNKNOWN_FILE;
        break;
    case AnnoState::YES:
        show_image_name = YES_FILE;
        break;
    case AnnoState::NO:
        show_image_name = NO_FILE;
        break;
    case AnnoState::UNSURE:
        show_image_name = UNSURE_FILE;
        break;
    }
    return show_image_name;
}

/**
 * @brief set_image 将图像设置到label上，图像自动根据label的大小来缩放
 * @param label
 * @param image
 */
void set_image(QLabel *label, const QPixmap &image) {
    if (image.width() <= label->width() && image.height() <= label->height()) {
        label->setPixmap(image);
        return;
    }
    float ratio(0.);
    ratio = 1. * label->width() / image.width();
    ratio = fmin( 1. * label->height() / image.height(), ratio );
    QPixmap m = image.scaled(static_cast<int>(image.width() * ratio), static_cast<int>(image.height() * ratio));
    label->setPixmap(m);
}

void set_image(QLabel *label, const std::string image_path) {
    QPixmap image(image_path.c_str());
    set_image(label, image);
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    updated(false)
{
    ui->setupUi(this);
    std::string type = "0";

    // 选择输入文件
    while (1) {
        QString file_name = QFileDialog::getOpenFileName(this, "choose a file to annotate", ".");
        if (file_name.isEmpty()) {
            int ok = QMessageBox::information(this, "choose a file to annotate", "Don't want to work now?", QMessageBox::Ok | QMessageBox::Cancel);
            if (ok == QMessageBox::Ok) {
                exit(0);
            }
            continue;
        }
        std::ifstream is(file_name.toStdString());
        is >> type;
        if ("0" == type) {
            std::string image_name;
            bool is_odd = true;
            while (is >> image_name) {
                if (is_odd) {
                    this->image_list_1.push_back(image_name);
                } else {
                    this->image_list_2.push_back(image_name);
                }
                is_odd = !is_odd;
            }
            is.close();
        } else if ("1" == type) {
            std::string image_name1, image_name2;
            int _label;
            while (is >> image_name1 >> image_name2 >> _label) {
                this->image_list_1.push_back(image_name1);
                this->image_list_2.push_back(image_name2);
                this->annotation_list.push_back(AnnoState(_label));
            }
            is.close();
        }
        if (image_list_1.size() != image_list_2.size()) {
            QMessageBox::information(this, "choose a file to annotate", "this image list is not even", QMessageBox::Ok);
            continue;
        }
        if (0 == image_list_1.size()) {
            QMessageBox::information(this, "choose a file to annotate", "this image list is empty", QMessageBox::Ok);
            continue;
        }
        break;
    }

    assert(image_list_1.size() == image_list_2.size());
    // 初始化其他参数
    this->total_pair_num = image_list_1.size();
    if ("0" == type) {
        this->current_idx = 0;
        std::vector<AnnoState> annotation_list(this->total_pair_num, AnnoState::UNKNOWN);
        this->annotation_list.swap(annotation_list);
    } else {
        for (int idx = this->annotation_list.size() - 1; idx >= 0; -- idx) {
            if (this->annotation_list[idx] != AnnoState::UNKNOWN) {
                this->current_idx = idx;
                std::cout << "this->current_idx: " << this->current_idx << std::endl;
                break;
            }
        }
    }

    display();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief MainWindow::display \n
 * 根据系统中的所有的变量来设置当前界面中的各个部分的内容
 */
void MainWindow::display() {

    if (this->current_idx >= this->total_pair_num) {
        QMessageBox::information(this, "annotation over", "Congratulations! You've finished all the job! Please save your work :)", QMessageBox::Ok);
        this->current_idx = this->total_pair_num - 1;
    }
    if (this->current_idx < 0) {
        QMessageBox::information(this, "annotation warning", "You must start at 0 (not a negative position, I konw you wanna challenge this app) :)", QMessageBox::Ok);
        this->current_idx = 0;
    }

    // 进度条
    this->ui->horizontalSlider_progress->setRange(0, this->total_pair_num - 1);
    this->ui->horizontalSlider_progress->setValue(this->current_idx);

    // 状态栏
    this->ui->statusBar->showMessage(QString((std::to_string(this->current_idx + 1) + " / " + std::to_string(this->total_pair_num)).c_str()));

    // 文件名
    std::string image_name_1 = this->image_list_1[this->current_idx];
    std::string image_base_name_1 = image_name_1.substr(image_name_1.find_last_of("/") + 1);
    std::string image_name_2 = this->image_list_2[this->current_idx];
    std::string image_base_name_2 = image_name_2.substr(image_name_2.find_last_of("/") + 1);
    this->ui->label_image_name_1->setText(image_base_name_1.c_str());
    this->ui->label_image_name_2->setText(image_base_name_2.c_str());

    // 显示图像
    set_image(this->ui->label_image_view_1, image_name_1);
    set_image(this->ui->label_image_view_2, image_name_2);

    // 显示标注结果
    std::string show_image_name = get_label_image(this->annotation_list[this->current_idx]);
    set_image(this->ui->label_image_compare_status, show_image_name);
    std::string prev_image_name = UNKNOWN_FILE;
    if ((this->current_idx - 1) >= 0) {
        prev_image_name = get_label_image(this->annotation_list[this->current_idx - 1]);
    }
    set_image(this->ui->label_prev_image_compare_status, prev_image_name);

    std::string next_image_name = UNKNOWN_FILE;
    if ((this->current_idx + 1) < static_cast<int>(this->annotation_list.size())) {
        next_image_name = get_label_image(this->annotation_list[this->current_idx + 1]);
    }
    set_image(this->ui->label_next_image_compare_status, next_image_name);

}

/**
 * @brief MainWindow::on_pushButton_save_clicked \n
 * 保存结果文件
 */
void MainWindow::on_pushButton_save_clicked()
{
    QString file_name = QFileDialog::getSaveFileName(this, "choose a file to save", ".");
    if (file_name.isEmpty()) {
        QMessageBox::information(this, "choose a file to save", "please enter a legal file name", QMessageBox::Ok);
        return;
    }
    std::ofstream os(file_name.toStdString());
    os << "1\n";
    for (int idx = 0; idx < static_cast<int>(this->annotation_list.size()); ++ idx) {
        os << this->image_list_1[idx] << " " << this->image_list_2[idx] << " " << this->annotation_list[idx] << "\n";
    }
    os.close();
    QMessageBox::information(this, "save", "save result success", QMessageBox::Ok);
    updated = false;
}

/**
 * @brief MainWindow::on_pushButton_ok_clicked
 * 标注为"匹配"
 */
void MainWindow::on_pushButton_ok_clicked()
{
    this->annotation_list[this->current_idx] = AnnoState::YES;
    ++ this->current_idx;
    updated = true;
    display();
}

/**
 * @brief MainWindow::on_pushButton_no_clicked
 * 标注为"不匹配"
 */
void MainWindow::on_pushButton_no_clicked()
{
    this->annotation_list[this->current_idx] = AnnoState::NO;
    ++ this->current_idx;
    updated = true;
    display();
}

/**
 * @brief MainWindow::on_pushButton_unsure_clicked
 * 标注为"不确定"
 */
void MainWindow::on_pushButton_unsure_clicked()
{
    this->annotation_list[this->current_idx] = AnnoState::UNSURE;
    ++ this->current_idx;
    updated = true;
    display();
}

/**
 * @brief MainWindow::on_pushButton_next_clicked
 * 移动到下一组
 */
void MainWindow::on_pushButton_next_clicked()
{
    ++ this->current_idx;
    display();
}

/**
 * @brief MainWindow::on_pushButton_prev_clicked
 * 移动到上一组
 */
void MainWindow::on_pushButton_prev_clicked()
{
    -- this->current_idx;
    display();
}

/**
 * @brief MainWindow::on_pushButton_switch_clicked
 * 交换两边的图片
 */
void MainWindow::on_pushButton_switch_clicked()
{
    std::string tmp = this->image_list_1[this->current_idx];
    this->image_list_1[this->current_idx] = this->image_list_2[this->current_idx];
    this->image_list_2[this->current_idx] = tmp;
    updated = true;
    display();
}

/**
 * @brief MainWindow::on_horizontalSlider_progress_sliderReleased
 * 拖放进度条，控制进度
 */
void MainWindow::on_horizontalSlider_progress_sliderReleased()
{
    int pos = this->ui->horizontalSlider_progress->value();
    this->current_idx = pos;
    this->display();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (updated) this->on_pushButton_save_clicked();
    QWidget::closeEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {

    /*
     * a : prev
     * d : next
     * w : yes
     * s ： no
     * space: unsure
     * c: switch
     *
     * j : prev
     * l : next
     * k : no
     * i : yes
     * m : swicth
     *
     * left: prev
     * right: next
     * up: yes
     * down: no
     */
    switch (event->key()) {
    case Qt::Key_A:
    case Qt::Key_J:
    case Qt::Key_Left:
        this->on_pushButton_prev_clicked();
        break;
    case Qt::Key_D:
    case Qt::Key_L:
    case Qt::Key_Right:
        this->on_pushButton_next_clicked();
        break;
    case Qt::Key_W:
    case Qt::Key_I:
    case Qt::Key_Up:
        this->on_pushButton_ok_clicked();
        break;
    case Qt::Key_S:
    case Qt::Key_K:
    case Qt::Key_Down:
        this->on_pushButton_no_clicked();
        break;
    case Qt::Key_C:
    case Qt::Key_M:
        this->on_pushButton_switch_clicked();
        break;
    case Qt::Key_Space:
        this->on_pushButton_unsure_clicked();
        break;
    }
}
