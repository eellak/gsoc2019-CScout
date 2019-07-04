import React,{Component} from 'react';

class Main extends Component {
    constructor(){
        super();
        this.state = {

        }
    }

    render() {

        return(
            <div>
                <div className="">
                    <h2> Files</h2>

                </div>

                <div>
                    <h2> Identifiers</h2>
                </div>

                <div>
                    <h2> Functions and Macros</h2>
                </div>

                <div>
                    <h2> File Dependancies</h2>
                </div>
            </div>
        )
    }
}
export default Main;