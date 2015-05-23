// This is the main DLL file.

#define dDOUBLE

#include "ode\ode.h"

static dWorldID world;
static dSpaceID space;
static dGeomID ground;

static dJointGroupID contactgroup;

static void nearCallback(void *data, dGeomID o1, dGeomID o2)
{
	dBodyID b1 = dGeomGetBody(o1);
	dBodyID b2 = dGeomGetBody(o2);
	dContact contact;
	contact.surface.mode = dContactBounce | dContactSoftCFM;
	// friction parameter
	contact.surface.mu = dInfinity;
	// bounce is the amount of "bouncyness".
	contact.surface.bounce = 0.3;
	// bounce_vel is the minimum incoming velocity to cause a bounce
	contact.surface.bounce_vel = 0.1;
	// constraint force mixing parameter
	contact.surface.soft_cfm = 0.001;

	if (int numc = dCollide(o1, o2, 1, &contact.geom, sizeof(dContact)))
	{
		dJointID c = dJointCreateContact(world, contactgroup, &contact);
		dJointAttach(c, b1, b2);
	}
}

// simulation loop


namespace Ode.NET{

	public ref class Tag
	{
	public:
		dBodyID body;
		dGeomID geom;
	};

	public interface class IDynamicObject
	{
		property array<double>^ SizeXYZ
		{
			array<double>^ get();
		}

		property array<double>^ CenterXYZ
		{
			array<double>^ get();
			void set(array<double>^);
		}

		property array<double>^ Rotation3x3
		{
			array<double>^ get();
			void set(array<double>^);
		}

		property Tag^ Tag;
	};

	public ref class Phys
	{
	public:
		static array<IDynamicObject^>^ _scene;

		static void simLoop(int pause)
		{
			System::Threading::Thread::Sleep(pause);

			// find collisions and add contact joints
			dSpaceCollide(space, NULL, &nearCallback);
			// step the simulation
			dWorldQuickStep(world, 0.1);

			// remove all contact joints
			dJointGroupEmpty(contactgroup);
			// redraw sphere at new location

			for each(auto obj in _scene)
			{
				auto phyObj = obj->Tag;
				auto T = dGeomGetPosition(phyObj->geom);
				auto R = dGeomGetRotation(phyObj->geom);

				obj->CenterXYZ = gcnew array < double > { T[0], T[1], T[2] };
				obj->Rotation3x3 = gcnew array < double >
				{
					R[0], R[1], R[2],
						R[4], R[5], R[6],
						R[8], R[9], R[10]
				};
			}
		}

		static void Run(array<IDynamicObject^>^ scene)
		{
			_scene = scene;

			dInitODE();

			world = dWorldCreate();
			space = dHashSpaceCreate(0);
			contactgroup = dJointGroupCreate(0);

			ground = dCreatePlane(space, 0, 0, 1, 0);
			dWorldSetGravity(world, 0, 0, -10);
			dWorldSetCFM(world, 1e-5);
			//dWorldSetERP(world, 0.1);

			for each(auto obj in _scene)
			{
				auto b = obj->SizeXYZ;

				// create object

				auto pObj = gcnew Tag();
				pObj->body = dBodyCreate(world);

				dMass m;
				dMassSetZero(&m);
				dMassSetBox(&m, 1, b[0], b[1], b[2]);
				dBodySetMass(pObj->body, &m);

				pObj->geom = dCreateBox(space, b[0], b[1], b[2]);
				dGeomSetBody(pObj->geom, pObj->body);
				// set initial position

				auto c = obj->CenterXYZ;

				dBodySetPosition(pObj->body, c[0], c[1], c[2]);

				obj->Tag = pObj;
			}

			for (int i = 0;; i++)
			{
				simLoop(20);
			}

			// clean up
			dJointGroupDestroy(contactgroup);
			dSpaceDestroy(space);
			dWorldDestroy(world);
			dCloseODE();
		}
	};

}
