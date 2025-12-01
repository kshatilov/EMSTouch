using System;
using System.Collections;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using Unity.VisualScripting;
using UnityEngine;

public class NetWrapper : MonoBehaviour
{

    private TcpClient client;
    private NetworkStream stream;
    public bool connected = false;

    // Start is called before the first frame update
    void Start()
    {
        try
        {
            client = new TcpClient();
            client.Connect("192.168.137.1", 1488);
            stream = client.GetStream();
            connected = true;
            Debug.Log("Connected to server.");

        }
        catch (Exception e)
        {
            Debug.LogError($"TCP Client Error: {e.Message}");
            connected = false;
        }
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    public void SendNetMessage(string message)
    {
        
        byte[] buffer = Encoding.ASCII.GetBytes(message);
        if (connected)
        {
            try
            {
                stream.Write(buffer, 0, buffer.Length);
            }
            catch (Exception e)
            {
                Debug.LogError($"TCP Client Error: {e.Message}");
                connected = false;
            }

        }
    }

    public void SendDstMessage(float dst)
    {
        Debug.Log("sending start message");
        string dst_str = "999";
        string message = "EMS_beg_dst_" + dst_str;
        this.SendNetMessage(message);
    }

    public void SendHndMessage(int id)
    {
        Debug.Log("sending start handshake message " + id.ToString());
        string dst_str = id.ToString();
        string message = "EMS_beg_hnd_" + dst_str;
        this.SendNetMessage(message);
    }

    private string pack(float value)
    {
        byte[] bytes = BitConverter.GetBytes(value);            // 4 bytes
        return Encoding.GetEncoding("ISO-8859-1").GetString(bytes);
    }
    public void SendPosMessage(float x, float y, float z)
    {
        //Debug.Log("sending pos message " + x  + " " + y + " " + z);
        var list = new List<byte>();
        list.AddRange(Encoding.ASCII.GetBytes("POS"));
        list.AddRange(BitConverter.GetBytes(x));
        list.AddRange(BitConverter.GetBytes(y));
        list.AddRange(BitConverter.GetBytes(z));

        byte[] t = list.ToArray();

        stream.Write(t, 0, t.Length);
    }

    public void SendEndMessage()
    {
        string message = "EMS_end_def_000";
        this.SendNetMessage(message);
    }
}
