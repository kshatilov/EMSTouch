using System.Collections;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;

public class DstManager : MonoBehaviour
{

    public GameObject targetObject;
    public OVRHand trackedHand;

    public NetWrapper server;
    public bool hasAnim;
    private bool detach;
    private bool sentEnd;


    public Animator animator;
    public string animationStateName = "Shaking Hands 2"; // Name of the state in Animator


    // Start is called before the first frame update
    void Start()
    {
        detach = true;
        sentEnd = false;
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    void FixedUpdate()
    {
        if (targetObject != null && trackedHand != null)
        {
            Vector3 handPosition = trackedHand.PointerPose.transform.position;
            Vector3 targetPosition = targetObject.transform.position;

            float distanceToHand = Vector3.Distance(handPosition, targetPosition);

            if (distanceToHand < 0.2f) 
            { 
                if (detach)
                {
                    Debug.Log("START HND");
                    server.SendHndMessage(Random.Range(101, 998));
                    animator.speed = 0.5f;
                    animator.Play(animationStateName, 0, 0f);
                    detach = false;
                } else {
                    server.SendPosMessage(handPosition.x, handPosition.y, handPosition.z);
                }
            }
            if (distanceToHand > 0.35f)
            {
                if (!detach)
                {
                    Debug.Log("END HND");
                    server.SendEndMessage();
                }
                detach = true;
            }
        }
    }

    public void Hover()
    {
        Debug.Log("DST Hover");
        
    }

    public void Blur()
    {

    }
}
