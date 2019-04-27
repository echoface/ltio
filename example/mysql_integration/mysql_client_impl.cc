#include "mysql_client_impl.h"

MysqlAsyncClientImpl::MysqlAsyncClientImpl();
MysqlAsyncClientImpl::~MysqlAsyncClientImpl();

RefQuerySession StartQuery() {

}

void MysqlAsyncClientImpl::PendingQuery(RefQuerySession& query) {
}

void MysqlAsyncClientImpl::ConnectionBroken(MysqlConnection* con) {

}
