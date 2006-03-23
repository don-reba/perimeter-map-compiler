using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Reflection;
using System.IO;

namespace TriggerEdit
{
	public class LinkHelpDialog : System.Windows.Forms.Form
	{
		//----------
		// interface
		//----------

		#region

		public LinkHelpDialog()
		{
			InitializeComponent();

			Assembly a = Assembly.GetExecutingAssembly();
			using (StreamReader reader = new StreamReader(a.GetManifestResourceStream(
						 "TriggerEdit.link help (en).rtf")))
			{
				en_text_rtb_.Rtf = reader.ReadToEnd();
			}
			using (StreamReader reader = new StreamReader(a.GetManifestResourceStream(
						 "TriggerEdit.link help (ru).rtf")))
			{
				ru_text_rtb_.Rtf = reader.ReadToEnd();
			}
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#endregion

		//---------------
		// implementation
		//---------------

		protected override bool ProcessDialogKey(Keys keyData)
		{
			if (keyData == Keys.Escape)
				Close();
			return base.ProcessDialogKey (keyData);
		}

		//----------
		// generated
		//----------

		#region data

		private System.ComponentModel.Container  components    = null;
		private System.Windows.Forms.RichTextBox en_text_rtb_;
		private System.Windows.Forms.TabControl  tabControl1;
		private System.Windows.Forms.TabPage     tabPage1;
		private System.Windows.Forms.TabPage     tabPage2;
		private System.Windows.Forms.RichTextBox ru_text_rtb_;

		#endregion

		#region code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.en_text_rtb_ = new System.Windows.Forms.RichTextBox();
			this.tabControl1 = new System.Windows.Forms.TabControl();
			this.tabPage1 = new System.Windows.Forms.TabPage();
			this.tabPage2 = new System.Windows.Forms.TabPage();
			this.ru_text_rtb_ = new System.Windows.Forms.RichTextBox();
			this.tabControl1.SuspendLayout();
			this.tabPage1.SuspendLayout();
			this.tabPage2.SuspendLayout();
			this.SuspendLayout();
			// 
			// en_text_rtb_
			// 
			this.en_text_rtb_.BackColor = System.Drawing.SystemColors.Control;
			this.en_text_rtb_.BorderStyle = System.Windows.Forms.BorderStyle.None;
			this.en_text_rtb_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.en_text_rtb_.Location = new System.Drawing.Point(8, 8);
			this.en_text_rtb_.Name = "en_text_rtb_";
			this.en_text_rtb_.ReadOnly = true;
			this.en_text_rtb_.Size = new System.Drawing.Size(448, 225);
			this.en_text_rtb_.TabIndex = 0;
			this.en_text_rtb_.Text = "";
			// 
			// tabControl1
			// 
			this.tabControl1.Appearance = System.Windows.Forms.TabAppearance.FlatButtons;
			this.tabControl1.Controls.Add(this.tabPage1);
			this.tabControl1.Controls.Add(this.tabPage2);
			this.tabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tabControl1.Location = new System.Drawing.Point(0, 0);
			this.tabControl1.Name = "tabControl1";
			this.tabControl1.SelectedIndex = 0;
			this.tabControl1.Size = new System.Drawing.Size(472, 270);
			this.tabControl1.TabIndex = 1;
			// 
			// tabPage1
			// 
			this.tabPage1.Controls.Add(this.en_text_rtb_);
			this.tabPage1.DockPadding.All = 8;
			this.tabPage1.Location = new System.Drawing.Point(4, 25);
			this.tabPage1.Name = "tabPage1";
			this.tabPage1.Size = new System.Drawing.Size(464, 241);
			this.tabPage1.TabIndex = 0;
			this.tabPage1.Text = "English";
			// 
			// tabPage2
			// 
			this.tabPage2.Controls.Add(this.ru_text_rtb_);
			this.tabPage2.DockPadding.All = 8;
			this.tabPage2.Location = new System.Drawing.Point(4, 25);
			this.tabPage2.Name = "tabPage2";
			this.tabPage2.Size = new System.Drawing.Size(464, 241);
			this.tabPage2.TabIndex = 1;
			this.tabPage2.Text = "Russian";
			// 
			// ru_text_rtb_
			// 
			this.ru_text_rtb_.BackColor = System.Drawing.SystemColors.Control;
			this.ru_text_rtb_.BorderStyle = System.Windows.Forms.BorderStyle.None;
			this.ru_text_rtb_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.ru_text_rtb_.Location = new System.Drawing.Point(8, 8);
			this.ru_text_rtb_.Name = "ru_text_rtb_";
			this.ru_text_rtb_.ReadOnly = true;
			this.ru_text_rtb_.Size = new System.Drawing.Size(448, 225);
			this.ru_text_rtb_.TabIndex = 1;
			this.ru_text_rtb_.Text = "";
			// 
			// LinkHelpDialog
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(472, 270);
			this.Controls.Add(this.tabControl1);
			this.Name = "LinkHelpDialog";
			this.Text = "Link Help";
			this.tabControl1.ResumeLayout(false);
			this.tabPage1.ResumeLayout(false);
			this.tabPage2.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion
	}
}
