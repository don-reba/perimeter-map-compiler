using System;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Drawing.Design;
using System.Reflection;
using System.Text;
using System.Windows.Forms;

namespace TriggerEdit
{
	public class BitEnumConverter : StringConverter
	{
		public override object ConvertTo(
			ITypeDescriptorContext context,
			System.Globalization.CultureInfo culture,
			object value,
			Type destinationType)
		{
			// check preconditions
			if (destinationType != typeof(string) || value.GetType() != typeof(BitEnum))
				return base.ConvertTo(context, culture, value, destinationType);
			// convert the list of set flags into a string
			BitEnum bit_enum = (BitEnum)value;
			ArrayList list   = bit_enum.GetList();
			StringBuilder result_builder = new StringBuilder();
			foreach (object field in list)
			{
				result_builder.Append(field.ToString());
				result_builder.Append(" | ");
			}
			if (result_builder.Length != 0)
				result_builder.Remove(result_builder.Length - 3, 3);
			return result_builder.ToString();
		}
		
		public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
		{
			return false;
		}

	}

	public class BitEnumEditor : UITypeEditor
	{
		public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
		{
			if (value == null)
				throw new ArgumentNullException();
			// cast value to BitEnum
			BitEnum bit_enum = (BitEnum)value;
			BitEnumEditorForm form = new BitEnumEditorForm();
			form.BitEnum = bit_enum;
			form.ShowDialog();
			return bit_enum;
		}

		public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
		{
			return UITypeEditorEditStyle.Modal;
		}
	}

	/// <summary>
	/// Enumeration with an arbitrary amount of flags.
	/// </summary>
	[ TypeConverter(typeof(BitEnumConverter)),
	  Editor(typeof(BitEnumEditor), typeof(UITypeEditor)) ]
	public class BitEnum
	{
		public BitEnum(Type enum_type)
		{
			if (!enum_type.IsEnum)
				throw new ArgumentException();
			// count the number of fields in the enumeration
			int field_count = 0;
			foreach (FieldInfo field in enum_type.GetFields())
				if (!field.IsSpecialName)
					++field_count;
			// initialize state
			enum_type_ = enum_type;
			bits_      = new BitArray(field_count);
		}

		public static BitEnum Parse(Type enum_type, string data)
		{
			string[] tokens = data.Split(new char[] {'|'});
			return Parse(enum_type, tokens);
		}

		public static BitEnum Parse(Type enum_type, string[] tokens)
		{
			BitEnum bit_enum = new BitEnum(enum_type);
			FieldInfo[] fields = enum_type.GetFields();
			foreach(string token in tokens)
				bit_enum[
					Array.IndexOf(
					fields,
					enum_type.GetField(token.Trim()))
					] = true;
			return bit_enum;
		}

		public void SetAll()
		{
			bits_.SetAll(true);
		}

		public void SetNone()
		{
			bits_.SetAll(false);
		}

		public bool this[int index]
		{
			get { return bits_[index]; }
			set { bits_[index] = value; }
		}

		public bool this[object index]
		{
			get { return this[Convert.ToInt32(index)]; }
			set { this[Convert.ToInt32(index)] = value; }
		}

		public Type GetEnumType()
		{
			return enum_type_;
		}

		public ArrayList GetList()
		{
			FieldInfo[] fields = enum_type_.GetFields();
			ArrayList list = new ArrayList();
			for (int i = 0; i != bits_.Length; ++i)
				if (bits_[i])
					list.Add(fields[i].Name);
			return list;
		}

		private Type     enum_type_;
		private BitArray bits_;
	}
}
