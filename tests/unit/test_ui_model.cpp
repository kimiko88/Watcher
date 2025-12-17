#include <gtest/gtest.h>
#include <QSignalSpy>
#include "cms/ui/ClientListModel.h"

using namespace cms::master;
using namespace cms::ui;

class ClientListModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        model = new ClientListModel();
    }

    void TearDown() override {
        delete model;
    }

    ClientListModel* model;
};

TEST_F(ClientListModelTest, InitialStateIsEmpty) {
    EXPECT_EQ(model->rowCount(), 0);
    EXPECT_EQ(model->columnCount(), 4);
}

TEST_F(ClientListModelTest, AddClientIncreasesCount) {
    ClientInfo client;
    client.id = "c1";
    client.hostname = "StudentPC";
    
    QSignalSpy spy(model, &QAbstractItemModel::rowsInserted);
    
    model->addClient(client);
    
    EXPECT_EQ(model->rowCount(), 1);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ClientListModelTest, RemoveClientDecreasesCount) {
    ClientInfo c1; c1.id = "c1";
    model->addClient(c1);
    
    QSignalSpy spy(model, &QAbstractItemModel::rowsRemoved);
    
    model->removeClient("c1");
    
    EXPECT_EQ(model->rowCount(), 0);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ClientListModelTest, UpdateReplacesAll) {
    std::vector<ClientInfo> list;
    list.push_back({"c1", "H1"});
    list.push_back({"c2", "H2"});
    
    QSignalSpy spy(model, &QAbstractItemModel::modelReset);
    
    model->updateClients(list);
    
    EXPECT_EQ(model->rowCount(), 2);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ClientListModelTest, DataRetrieval) {
    ClientInfo c1;
    c1.id = "c1";
    c1.hostname = "Target";
    c1.ip_address = "1.2.3.4";
    c1.state = ClientState::CONNECTED;
    
    model->addClient(c1);
    
    QModelIndex idx = model->index(0, 0);
    
    // Test custom roles
    EXPECT_EQ(model->data(idx, ClientListModel::HostnameRole).toString().toStdString(), "Target");
    EXPECT_EQ(model->data(idx, ClientListModel::IpRole).toString().toStdString(), "1.2.3.4");
    
    // Test display role (column 0 is hostname)
    EXPECT_EQ(model->data(model->index(0, 0), Qt::DisplayRole).toString().toStdString(), "Target");
}
