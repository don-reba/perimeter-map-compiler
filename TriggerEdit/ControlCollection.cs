using System;
using System.Collections;
using System.ComponentModel;

namespace TriggerEdit.Definitions
{
	[ TypeConverter(typeof(ExpandableObjectConverter)) ]
	public class Control
	{
		private ControlID controlID_;
		public  ControlID controlID
		{
			get { return controlID_; }
			set { controlID_ = value; }
		}
		private bool enabled_;
		public  bool enabled
		{
			get { return enabled_; }
			set { enabled_ = value; }
		}
		private bool visible_;
		public  bool visible
		{
			get { return visible_; }
			set { visible_ = value; }
		}
		private bool flashing_;
		public  bool flashing
		{
			get { return flashing_; }
			set { flashing_ = value; }
		}
		private int tabNumber_;
		public  int tabNumber
		{
			get { return tabNumber_; }
			set { tabNumber_ = value; }
		}
	}

	[ TypeConverter(typeof(ExpandableObjectConverter)) ]
	public class ControlCollection : CollectionBase, ICustomTypeDescriptor
	{
		protected class ControlPropertyDescriptor : PropertyDescriptor
		{
			public ControlPropertyDescriptor(Control control)
				:base("control", null)
			{
				control_ = control;
			}
			public override Type ComponentType
			{
				get { return typeof(Control); }
			}
			public override bool IsReadOnly
			{
				get { return true; }
			}
			public override Type PropertyType
			{
				get { return typeof(string); }
			}
			public override bool CanResetValue(object component)
			{
				return true;
			}
			public override object GetValue(object component)
			{
				return control_.controlID.ToString();
			}
			public override void ResetValue(object component)
			{
			}
			public override void SetValue(object component, object value)
			{
			}
			public override bool ShouldSerializeValue(object component)
			{
				return true;
			}
			public override string Name
			{
				get { return "control"; }
			}
			private Control control_;
		}

		#region Collection interface

		public Control this[int index]
		{
			get { return (Control)List[index]; }
			set { List[index] = value; }
		}
		public int Add(Control value)
		{
			return List.Add(value);
		}
		public void Remove(Control value)
		{
			List.Remove(value);
		}

		#endregion

		#region ICustomTypeDescriptor Members

		public TypeConverter GetConverter()
		{
			return TypeDescriptor.GetConverter(this, true);
		}

		public EventDescriptorCollection GetEvents(Attribute[] attributes)
		{
			return TypeDescriptor.GetEvents(this, true);
		}

		EventDescriptorCollection System.ComponentModel.ICustomTypeDescriptor.GetEvents()
		{
			return TypeDescriptor.GetEvents(this, true);
		}

		public string GetComponentName()
		{
			return TypeDescriptor.GetComponentName(this, true);
		}

		public object GetPropertyOwner(PropertyDescriptor pd)
		{
			return this;
		}

		public AttributeCollection GetAttributes()
		{
			return TypeDescriptor.GetAttributes(this, true);
		}

		public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
		{
			PropertyDescriptorCollection pds = new PropertyDescriptorCollection(null);
			foreach (Control control in List)
				pds.Add(new ControlPropertyDescriptor(control));
			return pds;
		}

		PropertyDescriptorCollection System.ComponentModel.ICustomTypeDescriptor.GetProperties()
		{
			return null;
		}

		public object GetEditor(Type editorBaseType)
		{
			return TypeDescriptor.GetEditor(this, editorBaseType, true);
		}

		public PropertyDescriptor GetDefaultProperty()
		{
			return TypeDescriptor.GetDefaultProperty(this, true);
		}

		public EventDescriptor GetDefaultEvent()
		{
			return TypeDescriptor.GetDefaultEvent(this, true);
		}

		public string GetClassName()
		{
			return TypeDescriptor.GetClassName(this, true);
		}

		#endregion
	}
}
