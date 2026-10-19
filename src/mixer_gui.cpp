#include "hush/operation_log.hpp"
#include "hush/project.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>

class MixerWindow final : public QMainWindow {
public:
    MixerWindow() {
        setWindowTitle("HUSH Cosmic Studio — Mixer"); resize(1050, 620);
        auto* root = new QWidget(this); auto* layout = new QVBoxLayout(root); auto* bar = new QHBoxLayout;
        auto* open = new QPushButton("Open Project…"); auto* save = new QPushButton("Save Project");
        auto* undo = new QPushButton("Undo"); auto* redo = new QPushButton("Redo");
        status_ = new QLabel("Offline mixer");
        bar->addWidget(open); bar->addWidget(save); bar->addWidget(undo); bar->addWidget(redo); bar->addStretch(); bar->addWidget(status_);
        layout->addLayout(bar); table_ = new QTableWidget(0, 6, root);
        table_->setHorizontalHeaderLabels({"Track", "Gain", "Pan", "Mute", "Solo", "Stable ID"});
        table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
        table_->verticalHeader()->hide(); table_->setAlternatingRowColors(true); layout->addWidget(table_); setCentralWidget(root);
        connect(open, &QPushButton::clicked, this, [this] { openProject(); });
        connect(save, &QPushButton::clicked, this, [this] { saveProject(); });
        connect(undo, &QPushButton::clicked, this, [this] { if (log_.undo(project_)) { refresh(); status_->setText("Undo applied"); } });
        connect(redo, &QPushButton::clicked, this, [this] { if (log_.redo(project_)) { refresh(); status_->setText("Redo applied"); } });
        project_.id = "hush-demo";
        project_.tracks = {{"track-1","Audio 1",1.0F,0.0F,false,false},
                            {"track-2","Audio 2",0.8F,-0.2F,false,false},
                            {"track-3","Reference",0.65F,0.2F,false,false}};
        refresh(); status_->setText("Demo session · unsaved");
    }
private:
    hush::Project project_;
    hush::OperationLog log_;
    QTableWidget* table_{};
    QLabel* status_{};
    void openProject() {
        const auto path = QFileDialog::getOpenFileName(this,"Open HUSH Project",{},"HUSH Project (*.json)");
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) { QMessageBox::warning(this,"Open failed",file.errorString()); return; }
        hush::Project candidate; hush::Error error;
        if (!hush::from_json(QString::fromUtf8(file.readAll()).toStdString(),candidate,error)) {
            QMessageBox::warning(this,"Invalid project",QString::fromStdString(error.message)); return;
        }
        project_=std::move(candidate); log_=hush::OperationLog{}; refresh();
        status_->setText("Loaded · "+QFileInfo(path).fileName());
    }
    void saveProject() {
        const auto path=QFileDialog::getSaveFileName(this,"Save HUSH Project","session.json","HUSH Project (*.json)");
        if(path.isEmpty()) return;
        QFile file(path);
        if(!file.open(QIODevice::WriteOnly|QIODevice::Truncate)){QMessageBox::warning(this,"Save failed",file.errorString());return;}
        const auto json=hush::to_json(project_);
        if(file.write(json.data(),static_cast<qint64>(json.size()))!=static_cast<qint64>(json.size())){
            QMessageBox::warning(this,"Save failed",file.errorString());return;
        }
        status_->setText("Saved · "+QFileInfo(path).fileName());
    }
    void change(const std::string& id,hush::OperationKind kind,float after) {
        const auto it=std::find_if(project_.tracks.begin(),project_.tracks.end(),[&](const hush::Track& t){return t.id==id;});
        if(it==project_.tracks.end()) return;
        float before=0.0F;
        switch(kind) {
        case hush::OperationKind::SetGain: before=it->gain; break;
        case hush::OperationKind::SetPan: before=it->pan; break;
        case hush::OperationKind::SetMute: before=it->mute?1.0F:0.0F; break;
        case hush::OperationKind::SetSolo: before=it->solo?1.0F:0.0F; break;
        }
        if(before!=after && log_.apply_and_append(project_,{id,kind,before,after}))
            status_->setText("Edited · undo history "+QString::number(log_.cursor()));
    }
    void refresh() {
        table_->setRowCount(static_cast<int>(project_.tracks.size()));
        for(std::size_t i=0;i<project_.tracks.size();++i) {
            const auto& t=project_.tracks[i]; const int row=static_cast<int>(i);
            auto* name=new QTableWidgetItem(QString::fromStdString(t.name)); name->setFlags(name->flags()&~Qt::ItemIsEditable); table_->setItem(row,0,name);
            auto* gain=new QSlider(Qt::Horizontal); gain->setRange(0,200); gain->setValue(static_cast<int>(t.gain*100.0F)); gain->setToolTip("Gain 0.00–2.00"); table_->setCellWidget(row,1,gain);
            auto* pan=new QSlider(Qt::Horizontal); pan->setRange(-100,100); pan->setValue(static_cast<int>(t.pan*100.0F)); pan->setToolTip("Pan −1.00 to +1.00"); table_->setCellWidget(row,2,pan);
            auto* mute=new QCheckBox("Mute"); mute->setChecked(t.mute); table_->setCellWidget(row,3,mute);
            auto* solo=new QCheckBox("Solo"); solo->setChecked(t.solo); table_->setCellWidget(row,4,solo);
            auto* id=new QTableWidgetItem(QString::fromStdString(t.id)); id->setFlags(id->flags()&~Qt::ItemIsEditable); table_->setItem(row,5,id);
            const auto trackId=t.id;
            connect(gain,&QSlider::sliderReleased,this,[this,gain,trackId]{change(trackId,hush::OperationKind::SetGain,gain->value()/100.0F);});
            connect(pan,&QSlider::sliderReleased,this,[this,pan,trackId]{change(trackId,hush::OperationKind::SetPan,pan->value()/100.0F);});
            connect(mute,&QCheckBox::clicked,this,[this,mute,trackId]{change(trackId,hush::OperationKind::SetMute,mute->isChecked()?1.0F:0.0F);});
            connect(solo,&QCheckBox::clicked,this,[this,solo,trackId]{change(trackId,hush::OperationKind::SetSolo,solo->isChecked()?1.0F:0.0F);});
        }
    }
};
int main(int argc,char** argv){QApplication app(argc,argv);MixerWindow window;window.show();return app.exec();}
