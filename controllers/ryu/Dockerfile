FROM python:2

RUN pip install --no-cache-dir networkx

WORKDIR /usr/src/app
COPY ryu .
RUN pip install .
ENV PYTHONPATH /usr/src/app/app:/usr/src/app/ryu/app
