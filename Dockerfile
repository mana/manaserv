FROM centos:7 as builder

RUN yum update -y
RUN yum install -y epel-release
RUN yum install -y cmake gcc-c++ make libxml2-devel physfs-devel sqlite-devel \
    lua-devel libsigc++20-devel

ADD . /source
WORKDIR /source

RUN cmake .
RUN make

FROM centos:7
RUN yum install -y epel-release
RUN yum install -y libxml2 physfs sqlite lua libsigc++20
COPY --from=builder /source/src/manaserv-account /app/
COPY --from=builder /source/src/manaserv-game /app/
COPY --from=builder /source/src/sql/sqlite/createTables.sql /app/
