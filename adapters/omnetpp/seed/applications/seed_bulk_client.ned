package seed.applications;

import inet.applications.ITCPApp;

simple SEEDBulkClient like ITCPApp
{
    gates:
        input tcpIn @labels(TCPCommand/up);
        output tcpOut @labels(TCPCommand/down);
}
